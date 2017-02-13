/*
 *  Copyright 2005-2016 Fabrice Colin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <stdlib.h>
#include <map>
#include <set>
#include <iostream>
#include <cstring>
#ifdef HAVE_BOOST_SPIRIT_CORE_HPP
#include <boost/spirit/core.hpp>
#include <boost/spirit/actor/push_back_actor.hpp>
#include <boost/spirit/actor/insert_at_actor.hpp>
#include <boost/spirit/utility/confix.hpp>
#else
#ifdef HAVE_BOOST_SPIRIT_HPP
#include <boost/spirit.hpp>
#include <boost/spirit/home/classic/actor/push_back_actor.hpp>
#include <boost/spirit/home/classic/actor/insert_at_actor.hpp>
#include <boost/spirit/home/classic/utility/confix.hpp>
#endif
#endif

#include "StringManip.h"
#include "Url.h"
#include "HtmlFilter.h"
#include "FilterFactory.h"
#include "FileCollector.h"
#include "FilterUtils.h"
#include "SherlockParser.h"

using std::clog;
using std::endl;
using std::string;
using std::vector;
using std::map;
using std::set;
using std::exception;

using namespace boost::spirit;

// A function object to lower case map keys with for_each()
struct LowerAndCopy
{
	public:
		LowerAndCopy(map<string, string> &other) :
			m_other(other)
		{
		}

		void operator()(map<string, string>::value_type &p)
		{
			m_other[StringManip::toLowerCase(p.first)] = p.second;
		}

		map<string, string> &m_other;

};

struct plugin_skip_grammar : public grammar<plugin_skip_grammar>
{
	template <typename ScannerT>
	struct definition
	{
		definition(plugin_skip_grammar const &self)
		{
			// Skip all spaces and comments, starting with a #
			// FIXME: make sure comments start at the beginning of the line !
			skip = space_p | (ch_p('#') >> *(anychar_p - ch_p('\n')) >> ch_p('\n'));
		}
	
		rule<ScannerT> skip;
	
		rule<ScannerT> const& start() const
		{
			return skip;
		}
	};
};

/**
  * A complete but lax grammar for Sherlock plugins.
  * For instance, it doesn't mind if INPUT has a NAME but no VALUE.
  * More importantly, it doesn't enforce types, eg FACTOR should be an integer.
  */
struct plugin_grammar : public grammar<plugin_grammar>
{
	plugin_grammar(map<string, string> &searchParams,
		map<string, string> &interpretParams,
		map<string, string> &inputItems,
		string &userInput, string &nextInput,
		string &nextFactor,
		string &nextValue) :
		m_searchParams(searchParams),
		m_interpretParams(interpretParams),
		m_inputItems(inputItems),
		m_userInput(userInput),
		m_nextInput(nextInput),
		m_nextFactor(nextFactor),
		m_nextValue(nextValue)
	{
	}

	template <typename ScannerT>
	struct definition
	{
		definition(plugin_grammar const &self)
		{
			// Start
			search_plugin = search_header >> input_elements >> search_footer >> rest;

			// All items have a name and an optionally-quoted value, separated by =
			end_of_name = ch_p('=');
			any_name = *(~ch_p('>') - end_of_name);
			any_value_without_quotes = lexeme_d[*(~ch_p('>') - ch_p('\n'))];
			any_value = ch_p('\'') >> (*(~ch_p('\'')))[assign_a(unquotedValue)] >> ch_p('\'') |
				ch_p('"') >> (*(~ch_p('"')))[assign_a(unquotedValue)] >> ch_p('"') |
				any_value_without_quotes[assign_a(unquotedValue)];

			// SEARCH attributes are items
			// There should be only one SEARCH tag
			search_item = (any_name[assign_a(itemName)]
				>> ch_p('=') >> any_value[assign_a(itemValue, unquotedValue)])
				[insert_at_a(self.m_searchParams, itemName, itemValue)];

			// SEARCH may have any number of attributes
			search_header = ch_p('<') >> as_lower_d[str_p("search")] >> *search_item >> ch_p('>');

			// INPUT
			input_item_name = as_lower_d[str_p("name")] >> ch_p('=')
				>> any_value[assign_a(itemName, unquotedValue)];
			input_item_value = as_lower_d[str_p("value")] >> ch_p('=')
				>> any_value[assign_a(itemValue, unquotedValue)];
			input_item_user = as_lower_d[str_p("user")];
			input_item_factor = as_lower_d[str_p("factor")]
				>> ch_p('=') >> any_value[assign_a(itemValue, unquotedValue)];

			// INPUT tags have name and value items; one is marked with USER
			input_item = input_item_name |
				input_item_value |
				input_item_user[assign_a(self.m_userInput, itemName)];

			input_element = (ch_p('<') >> as_lower_d[str_p("input")] >> *input_item >> ch_p('>'))
				[insert_at_a(self.m_inputItems, itemName, itemValue)];

			// INPUTPREV tags have name and either factor or value items
			// There should be only one INPUTPREV tag
			// FIXME: save those
			inputprev_item = input_item_name |
				input_item_factor |
				input_item_value;

			inputprev_element = ch_p('<') >> as_lower_d[str_p("inputprev")] >> *inputprev_item >> ch_p('>');

			// INPUTNEXT tags have name and either factor or value items
			// There should be only one INPUTNEXT tag
			inputnext_item = input_item_name[assign_a(self.m_nextInput, itemName)] |
				input_item_factor[assign_a(self.m_nextFactor, itemValue)] |
				input_item_value[assign_a(self.m_nextValue, itemValue)];

			inputnext_element = ch_p('<') >> as_lower_d[str_p("inputnext")] >> *inputnext_item >> ch_p('>');

			// INTERPRET tags have varied types of items
			// There should be only one INTERPRET tag
			interpret_item = (any_name[assign_a(itemName)]
				>> ch_p('=') >> any_value[assign_a(itemValue, unquotedValue)])
				[insert_at_a(self.m_interpretParams, itemName, itemValue)];

			interpret_element = ch_p('<') >> as_lower_d[str_p("interpret")] >> *interpret_item >> ch_p('>');

			// INPUT, INPUTNEXT and INTERPRET may appear in any order
			input_elements = *(input_element |
				inputprev_element |
				inputnext_element |
				interpret_element);

			// SEARCH has a closing tag
			search_footer =  ch_p('<') >> ch_p('/') >> as_lower_d[str_p("search")] >> ch_p('>');

			// Rest
			rest = *anychar_p;
		}

		string unquotedValue, itemName, itemValue;		
		rule<ScannerT> search_plugin, search_header, search_footer, rest;
		rule<ScannerT> end_of_name, any_name, any_value_without_quotes, any_value, search_item;
		rule<ScannerT> input_elements, input_element, inputprev_element, inputnext_element, interpret_element;
		rule<ScannerT> input_item_name, input_item_value, input_item_user, input_item_factor;
		rule<ScannerT> input_item, inputprev_item, inputnext_item, interpret_item;

		rule<ScannerT> const& start() const
		{
			return search_plugin;
		}
	};

	map<string, string> &m_searchParams;
	map<string, string> &m_interpretParams;
	map<string, string> &m_inputItems;
	string &m_userInput;
	string &m_nextInput;
	string &m_nextFactor;
	string &m_nextValue;

};

SherlockResponseParser::SherlockResponseParser() :
	ResponseParserInterface(),
	m_skipLocal(true)
{
}

SherlockResponseParser::~SherlockResponseParser()
{
}

bool SherlockResponseParser::parse(const Document *pResponseDoc, vector<DocumentInfo> &resultsList,
	unsigned int &totalResults, unsigned int &firstResultIndex, string &charset) const
{
	float pseudoScore = 100;
	off_t contentLen = 0;
	bool foundResult = false;

	if ((pResponseDoc == NULL) ||
		(pResponseDoc->getData(contentLen) == NULL) ||
		(contentLen == 0))
	{
		return false;
	}

	// Can we get the charset ?
	Dijon::HtmlFilter htmlFilter;
	htmlFilter.set_mime_type("text/html");
	if (FilterUtils::feedFilter(*pResponseDoc, &htmlFilter) == true)
	{
		const map<string, string> &metaData = htmlFilter.get_meta_data();
		map<string, string>::const_iterator charsetIter = metaData.find("charset");

		if (charsetIter != metaData.end())
		{
			charset = charsetIter->second;
#ifdef DEBUG
			clog << "SherlockResponseParser::parse: response charset is " << charset << endl;
#endif
		}
	}

	// These two are the minimum we need
	if ((m_resultItemStart.empty() == true) ||
		(m_resultItemEnd.empty() == true))
	{
#ifdef DEBUG
		clog << "SherlockResponseParser::parse: incomplete properties" << endl;
#endif
		return false;
	}

	// Extract the results list
#ifdef DEBUG
	clog << "SherlockResponseParser::parse: getting results list ("
		<< m_resultListStart << ", " << m_resultListEnd << ")" << endl;
#endif
	const char *pContent = pResponseDoc->getData(contentLen);
	string resultList = StringManip::extractField(pContent, m_resultListStart, m_resultListEnd);
	if (resultList.empty() == true)
	{
		resultList = string(pContent, contentLen);
	}

	// Extract results
	string::size_type endPos = 0;
#ifdef DEBUG
	clog << "SherlockResponseParser::parse: getting first result ("
		<< m_resultItemStart << ", " << m_resultItemEnd << ")" << endl;
#endif
	string resultItem = StringManip::extractField(resultList,
		m_resultItemStart, m_resultItemEnd, endPos);
	while (resultItem.empty() == false)
	{
		string contentType, url, name, extract;

#ifdef DEBUG
		clog << "SherlockResponseParser::parse: candidate chunk \""
			<< resultItem << "\"" << endl;
#endif
		contentType = pResponseDoc->getType();
		if (strncasecmp(contentType.c_str(), "text/html", 9) == 0)
		{
			Document chunkDoc("", "", contentType, "");
			string htmlChunk(resultItem);

			// The chunk may contain truncated tags, get rid of them !
			string::size_type firstOpen = htmlChunk.find('<');
			string::size_type firstClose = htmlChunk.find('>');
			if (firstClose != string::npos)
			{
				if ((firstOpen == string::npos) ||
					(firstClose < firstOpen))
				{
					htmlChunk.erase(0, firstClose + 1);
				}
			}
			string::size_type lastClose = htmlChunk.find_last_of(">");
			string::size_type lastOpen = htmlChunk.find_last_of("<");
			if (lastOpen != string::npos)
			{
				if ((lastClose == string::npos) ||
					(lastOpen > lastClose))
				{
					htmlChunk.erase(lastOpen);
				}
			}

			// Wrap input
			string dummyHtml("<html><head><meta HTTP-EQUIV=\"content-type\" CONTENT=\"");
			dummyHtml += pResponseDoc->getType();
			dummyHtml += "\"></head><body>";
			dummyHtml += htmlChunk;
			dummyHtml += "</body></html>";
#ifdef DEBUG
			clog << "SherlockResponseParser::parse: wrapped chunk \""
				<< dummyHtml << "\"" << endl;
#endif
			chunkDoc.setData(dummyHtml.c_str(), dummyHtml.length());

			// Feed this chunk to the filter
			Dijon::HtmlFilter chunkFilter;
			set<Dijon::Link> chunkLinks;
			htmlFilter.set_mime_type("text/html");
			if ((FilterUtils::feedFilter(chunkDoc, &chunkFilter) == true) &&
				(chunkFilter.get_links(chunkLinks) == true) &&
				(chunkFilter.next_document() == true))
			{
				unsigned int endOfFirstLink = 0, startOfSecondLink = 0, endOfSecondLink = 0, startOfThirdLink = 0;

				// The result's URL and title should be given by the first link
				for (set<Dijon::Link>::iterator linkIter = chunkLinks.begin(); linkIter != chunkLinks.end(); ++linkIter)
				{
					if (linkIter->m_index == 0)
					{
						url = linkIter->m_url;
						name = linkIter->m_name;
#ifdef DEBUG
						clog << "SherlockResponseParser::parse: first link in chunk is "
							<< url << endl;
#endif
						endOfFirstLink = linkIter->m_endPos;
					}
					else if (linkIter->m_index == 1)
					{
						startOfSecondLink = linkIter->m_startPos;
						endOfSecondLink = linkIter->m_endPos;
					}
					else if (linkIter->m_index == 2)
					{
						startOfThirdLink = linkIter->m_startPos;
					}
				}

				// Any extract ?
				const map<string, string> &metaData = chunkFilter.get_meta_data();
				map<string, string>::const_iterator abstractIter = metaData.find("abstract");
				if (abstractIter == metaData.end())
				{
					extract = FilterUtils::stripMarkup(resultItem);
					StringManip::trimSpaces(extract);
				}
				else
				{
					extract = abstractIter->second;
				}
			}
		}
		else
		{
			// This is not HTML
			// Use extended attributes
			if ((m_resultTitleStart.empty() == false) &&
				(m_resultTitleEnd.empty() == false))
			{
				name = StringManip::extractField(resultItem,
					m_resultTitleStart, m_resultTitleEnd);
			}

			if ((m_resultLinkStart.empty() == false) &&
				(m_resultLinkEnd.empty() == false))
			{
				url = StringManip::extractField(resultItem,
					m_resultLinkStart, m_resultLinkEnd);
			}

			if ((m_resultExtractStart.empty() == false) &&
				(m_resultExtractEnd.empty() == false))
			{
				extract = StringManip::extractField(resultItem,
					m_resultExtractStart, m_resultExtractEnd);
			}
		}

		if (url.empty() == false)
		{
			// FIXME: look for a interpret/baseurl tag, see https://bugzilla.mozilla.org/show_bug.cgi?id=65453
			// FIXME: obey m_skipLocal
			DocumentInfo result(name, url, "", "");
			result.setExtract(extract);
			result.setScore(pseudoScore);

			resultsList.push_back(result);
			--pseudoScore;
			foundResult = true;
			if (resultsList.size() == totalResults)
			{
				// Enough results
				break;
			}
		}

		// Next
		endPos += m_resultItemEnd.length();
		resultItem = StringManip::extractField(resultList,
			m_resultItemStart, m_resultItemEnd, endPos);
	}

	return foundResult;
}

pthread_mutex_t SherlockParser::m_mutex = PTHREAD_MUTEX_INITIALIZER;

SherlockParser::SherlockParser(const string &fileName) :
	PluginParserInterface(fileName)
{
}

SherlockParser::~SherlockParser()
{
}

ResponseParserInterface *SherlockParser::parse(SearchPluginProperties &properties,
	bool minimal)
{
	FileCollector fileCollect;
	DocumentInfo docInfo("Sherlock Source", string("file://") + m_fileName,
		"text/plain", "");

	// Get the definition file
	Document *pPluginDoc = fileCollect.retrieveUrl(docInfo);
	if (pPluginDoc == NULL)
	{
#ifdef DEBUG
		clog << "SherlockParser::parse: couldn't load " << m_fileName << endl;
#endif
		return NULL;
	}

	off_t dataLength;
	const char *pData = pPluginDoc->getData(dataLength);
	if ((pData == NULL) ||
		(dataLength == 0))
	{
		delete pPluginDoc;
		return NULL;
	}

	map<string, string> searchParams, interpretParams, inputItems;
	string userInput, nextInput, nextFactor, nextValue;
	bool parsedPlugin = false;

	if (pthread_mutex_lock(&m_mutex) == 0)
	{
		try
		{
			plugin_skip_grammar skip;
			plugin_grammar plugin(searchParams, interpretParams, inputItems,
				userInput, nextInput, nextFactor, nextValue);

			parse_info<> parseInfo = boost::spirit::parse(pData, plugin, skip);
			parsedPlugin = parseInfo.hit;
		}
		catch (const exception &e)
		{
#ifdef DEBUG
			clog << "SherlockParser::parse: caught exception ! " << e.what() << endl;
#endif
			parsedPlugin = false;
		}
		catch (...)
		{
#ifdef DEBUG
			clog << "SherlockParser::parse: caught unknown exception !" << endl;
#endif
			parsedPlugin = false;
		}

		pthread_mutex_unlock(&m_mutex);
	}

	// We are done with the document
	delete pPluginDoc;

	SherlockResponseParser *pResponseParser = NULL;

	if (parsedPlugin == true)
	{
		map<string, string> lowSearchParams, lowInterpretParams;

		pResponseParser = new SherlockResponseParser();

		LowerAndCopy lowCopy1(lowSearchParams);
		for_each(searchParams.begin(), searchParams.end(), lowCopy1);
		LowerAndCopy lowCopy2(lowInterpretParams);
		for_each(interpretParams.begin(), interpretParams.end(), lowCopy2);

		// Response
		properties.m_response = SearchPluginProperties::HTML_RESPONSE;
		// Method
		properties.m_method = SearchPluginProperties::GET_METHOD;

		// Name
		map<string, string>::iterator mapIter = lowSearchParams.find("name");
		if (mapIter != lowSearchParams.end())
		{
			properties.m_longName = mapIter->second;
		}

		// Channel
		mapIter = lowSearchParams.find("routetype");
		if (mapIter != lowSearchParams.end())
		{
			properties.m_channel = mapIter->second;
		}

		if (userInput.empty() == false)
		{
			// Remove the user input tag from the input tags map
			mapIter = inputItems.find(userInput);
			if (mapIter != inputItems.end())
			{
				inputItems.erase(mapIter);
			}
#ifdef DEBUG
			else clog << "SherlockParser::parse: couldn't remove user input item" << endl;
#endif

			properties.m_variableParameters[SearchPluginProperties::SEARCH_TERMS_PARAM] = userInput;
		}
		for (map<string, string>::iterator iter = inputItems.begin();
			iter != inputItems.end(); ++iter)
		{
#ifdef DEBUG
			clog << "SherlockParser::parse: " << iter->first << "=" << iter->second << endl;
#endif
			if (iter->second.substr(0, 5) == "EDIT:")
			{
				// This is user editable
				properties.m_editableParameters[iter->first] = iter->second.substr(5);
			}
			else
			{
				// Append to the remainder
				if (properties.m_remainder.empty() == false)
				{
					properties.m_remainder += "&";
				}
				properties.m_remainder += iter->first;
				properties.m_remainder += "=";
				properties.m_remainder += iter->second;
			}
		}

		if (minimal == false)
		{
			// URL
			mapIter = lowSearchParams.find("action");
			if (mapIter != lowSearchParams.end())
			{
				properties.m_baseUrl = mapIter->second;
			}

			// Response
			mapIter = lowInterpretParams.find("resultliststart");
			if (mapIter != lowInterpretParams.end())
			{
				pResponseParser->m_resultListStart = StringManip::replaceSubString(mapIter->second, "\\n", "\n");
			}

			mapIter = lowInterpretParams.find("resultlistend");
			if (mapIter != lowInterpretParams.end())
			{
				pResponseParser->m_resultListEnd = StringManip::replaceSubString(mapIter->second, "\\n", "\n");
			}

			mapIter = lowInterpretParams.find("resultitemstart");
			if (mapIter != lowInterpretParams.end())
			{
				pResponseParser->m_resultItemStart = StringManip::replaceSubString(mapIter->second, "\\n", "\n");
			}

			mapIter = lowInterpretParams.find("resultitemend");
			if (mapIter != lowInterpretParams.end())
			{
				pResponseParser->m_resultItemEnd = StringManip::replaceSubString(mapIter->second, "\\n", "\n");
			}

			mapIter = lowInterpretParams.find("resulttitlestart");
			if (mapIter != lowInterpretParams.end())
			{
				pResponseParser->m_resultTitleStart = mapIter->second;
			}

			mapIter = lowInterpretParams.find("resulttitleend");
			if (mapIter != lowInterpretParams.end())
			{
				pResponseParser->m_resultTitleEnd = mapIter->second;
			}

			mapIter = lowInterpretParams.find("resultlinkstart");
			if (mapIter != lowInterpretParams.end())
			{
				pResponseParser->m_resultLinkStart = mapIter->second;
			}

			mapIter = lowInterpretParams.find("resultlinkend");
			if (mapIter != lowInterpretParams.end())
			{
				pResponseParser->m_resultLinkEnd = mapIter->second;
			}

			mapIter = lowInterpretParams.find("resultextractstart");
			if (mapIter != lowInterpretParams.end())
			{
				pResponseParser->m_resultExtractStart = mapIter->second;
			}

			mapIter = lowInterpretParams.find("resultextractend");
			if (mapIter != lowInterpretParams.end())
			{
				pResponseParser->m_resultExtractEnd = mapIter->second;
			}

			mapIter = lowInterpretParams.find("skiplocal");
			if (mapIter != lowInterpretParams.end())
			{
				if (mapIter->second == "false")
				{
					pResponseParser->m_skipLocal = false;
				}
			}

			// Here we differ from how Mozilla uses these parameters
			// Normally, either factor or value is used, but we use value
			// as the parameter's initial value
			if (nextFactor.empty() == false)
			{
				properties.m_variableParameters[SearchPluginProperties::START_PAGE_PARAM] = nextInput;
				properties.m_scrolling = SearchPluginProperties::PER_PAGE;
				// What Sherlock calls a factor is actually an increment
				properties.m_nextIncrement = (unsigned int)atoi(nextFactor.c_str());
			}
			else
			{
				// Assume INPUTNEXT allows to specify a number of results
				// Not sure if this is how Sherlock/Mozilla interpret this
				properties.m_variableParameters[SearchPluginProperties::COUNT_PARAM] = nextInput;
				properties.m_scrolling = SearchPluginProperties::PER_INDEX;
				properties.m_nextIncrement = 0;
			}
			if (nextValue.empty() == false)
			{
				properties.m_nextBase = (unsigned int)atoi(nextValue.c_str());
			}
			else
			{
				properties.m_nextBase = 0;
			}
		}
	}

	return pResponseParser;
}
