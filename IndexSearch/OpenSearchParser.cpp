/*
 *  Copyright 2005-2009 Fabrice Colin
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <glibmm/thread.h>
#include <glibmm/convert.h>
#include <libxml++/libxml++.h>
#include <libxml++/nodes/cdatanode.h>

#include "StringManip.h"
#include "FilterUtils.h"
#include "OpenSearchParser.h"

using namespace std;
using namespace Glib;
using namespace xmlpp;

static ustring getNodeContent(const Node *pNode)
{
	if (pNode == NULL)
	{
		return "";
	}

	// Is it an element ?
	const Element *pElem = dynamic_cast<const Element*>(pNode);
	if (pElem != NULL)
	{
#ifdef HAS_LIBXMLPP026
		const TextNode *pText = pElem->get_child_content();
#else
		const TextNode *pText = pElem->get_child_text();
#endif
		if (pText == NULL)
		{
			// Maybe the text is given as CDATA
			const Node::NodeList childNodes = pNode->get_children();
			if (childNodes.size() == 1)
			{
				// Is it CDATA ?
				const CdataNode *pContent = dynamic_cast<const CdataNode*>(*childNodes.begin());
				if (pContent != NULL)
				{
					return pContent->get_content();
				}
			}

			return "";
		}

		return pText->get_content();
	}

	return "";
}

OpenSearchResponseParser::OpenSearchResponseParser(bool rssResponse) :
	ResponseParserInterface(),
	m_rssResponse(rssResponse)
{
}

OpenSearchResponseParser::~OpenSearchResponseParser()
{
}

bool OpenSearchResponseParser::parse(const ::Document *pResponseDoc, vector<DocumentInfo> &resultsList,
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

	// Make sure the response MIME type is sensible
	string mimeType = pResponseDoc->getType();
	if ((mimeType.empty() == false) &&
		(mimeType.find("xml") == string::npos))
	{
		clog << "OpenSearchResponseParser::parse: response is not XML" << endl;
		return false;
	}

	const char *pContent = pResponseDoc->getData(contentLen);
	try
	{
		bool loadFeed = false;

		DomParser parser;
		parser.set_substitute_entities(true);
		parser.parse_memory_raw((const unsigned char *)pContent, (Parser::size_type)contentLen);
		xmlpp::Document *pDocument = parser.get_document();
		if (pDocument == NULL)
		{
			return false;
		}

		ustring encoding(pDocument->get_encoding());
		if (encoding.empty() == false)
		{
			charset = encoding;
#ifdef DEBUG
			clog << "OpenSearchResponseParser::parse: response charset is " << charset << endl;
#endif
		}

		Node *pNode = pDocument->get_root_node();
		Element *pRootElem = dynamic_cast<Element *>(pNode);
		if (pRootElem == NULL)
		{
			return false;
		}
		// Check the top-level element is what we expect
		ustring rootNodeName = pRootElem->get_name();
		if (m_rssResponse == true)
		{
			if (rootNodeName == "rss")
			{
				const Node::NodeList rssChildNodes = pRootElem->get_children();
				for (Node::NodeList::const_iterator rssIter = rssChildNodes.begin();
					rssIter != rssChildNodes.end(); ++rssIter)
				{
					Node *pRssNode = (*rssIter);
					Element *pRssElem = dynamic_cast<Element*>(pRssNode);
					if (pRssElem != NULL)
					{
						if (pRssElem->get_name() == "channel")
						{
							pRootElem = pRssElem;
							loadFeed = true;
							break;
						}
					}
				}
			}
		}
		else
		{
			if (rootNodeName != "feed")
			{
				return false;
			}
			loadFeed = true;
		}

		if (loadFeed == false)
		{
#ifdef DEBUG
			clog << "OpenSearchResponseParser::parse: error on root node "
				<< rootNodeName << endl;
#endif
			return false;
		}

		// RSS
		ustring itemNode("item");
		ustring descriptionNode("description");
		if (m_rssResponse == false)
		{
			// Atom
			itemNode = "entry";
			descriptionNode = "content";
		}

		// Go through the subnodes
		const Node::NodeList childNodes = pRootElem->get_children();
		for (Node::NodeList::const_iterator iter = childNodes.begin();
			iter != childNodes.end(); ++iter)
		{
			Node *pChildNode = (*iter);
			ustring nodeName(pChildNode->get_name());
			ustring nodeContent(getNodeContent(pChildNode));

			// Is this an OpenSearch extension ?
			// FIXME: make sure namespace is opensearch
			if (nodeName == "totalResults")
			{
				if (nodeContent.empty() == false)
				{
					totalResults = min((unsigned int)atoi(nodeContent.c_str()), totalResults);
#ifdef DEBUG
					clog << "OpenSearchResponseParser::parse: total results "
						<< totalResults << endl;
#endif
				}
			}
			else if (nodeName == "startIndex")
			{
				if (nodeContent.empty() == false)
				{
					firstResultIndex = (unsigned int)atoi(nodeContent.c_str());
#ifdef DEBUG
					clog << "OpenSearchResponseParser::parse: first result index "
						<< firstResultIndex << endl;
#endif
				}
			}

			if (nodeName != itemNode)
			{
				continue;
			}

			// Go through the item's subnodes
			ustring title, url, extract;
			const Node::NodeList itemChildNodes = pChildNode->get_children();
			for (Node::NodeList::const_iterator itemIter = itemChildNodes.begin();
				itemIter != itemChildNodes.end(); ++itemIter)
			{
				Node *pItemNode = (*itemIter);
				Element *pItemElem = dynamic_cast<Element*>(pItemNode);
				if (pItemElem == NULL)
				{
					continue;
				}

				ustring itemNodeName = pItemNode->get_name();
				if (itemNodeName == "title")
				{
					title = getNodeContent(pItemNode);
				}
				else if (itemNodeName == "link")
				{
					if (m_rssResponse == true)
					{
						url = getNodeContent(pItemNode);
					}
					else
					{
						Attribute *pAttr = pItemElem->get_attribute("href");
						if (pAttr != NULL)
						{
							url = pAttr->get_value();
						}
					}
				}
				else if (itemNodeName == descriptionNode)
				{
					extract = getNodeContent(pItemNode);
				}
			}

			// The extract may contain HTML
			if ((extract.find("<") != string::npos) &&
				(extract.find(">") != string::npos))
			{
				// Wrap the extract
				ustring dummyHtml("<html><head><meta HTTP-EQUIV=\"content-type\" CONTENT=\"text/html\"></head><body>");
				dummyHtml += extract;
				dummyHtml += "</body></html>";

				extract = FilterUtils::stripMarkup(dummyHtml);
			}

			DocumentInfo result(title, url, "", "");
			result.setExtract(extract);
			result.setScore(pseudoScore);

			resultsList.push_back(result);
			--pseudoScore;
			foundResult = true;
			if (resultsList.size() >= totalResults)
			{
				// Enough results
				break;
			}
		}
	}
	catch (const std::exception& ex)
	{
#ifdef DEBUG
		clog << "OpenSearchResponseParser::parse: caught exception: " << ex.what() << endl;
#endif
		foundResult = false;
	}

	return foundResult;
}

OpenSearchParser::OpenSearchParser(const string &fileName) :
	PluginParserInterface(fileName)
{
}

OpenSearchParser::~OpenSearchParser()
{
}

ResponseParserInterface *OpenSearchParser::parse(SearchPluginProperties &properties,
	bool minimal)
{
	struct stat fileStat;
	bool rssResponse = true, success = true;

	if ((m_fileName.empty() == true) ||
		(stat(m_fileName.c_str(), &fileStat) != 0) ||
		(!S_ISREG(fileStat.st_mode)))
	{
		return NULL;
	}

	try
	{
		DomParser parser;
		parser.set_substitute_entities(true);
		parser.parse_file(m_fileName);
		xmlpp::Document *pDocument = parser.get_document();
		if (pDocument == NULL)
		{
			return NULL;
		}

		Node *pNode = pDocument->get_root_node();
		Element *pRootElem = dynamic_cast<Element *>(pNode);
		if (pRootElem == NULL)
		{
			return NULL;
		}
		// Check the top-level element is what we expect
		// MozSearch is very much like OpenSearch Description
		ustring rootNodeName = pRootElem->get_name();
		if ((rootNodeName != "OpenSearchDescription") &&
			(rootNodeName != "SearchPlugin"))
		{
#ifdef DEBUG
			clog << "OpenSearchParser::parse: wrong root node " << rootNodeName << endl;
#endif
			return NULL;
		}

		// Go through the subnodes
		const Node::NodeList childNodes = pRootElem->get_children();
		if (childNodes.empty() == false)
		{
			for (Node::NodeList::const_iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
			{
				Node *pChildNode = (*iter);
				Element *pElem = dynamic_cast<Element*>(pChildNode);
				if (pElem == NULL)
				{
					continue;
				}

				ustring nodeName(pChildNode->get_name());
				ustring nodeContent(getNodeContent(pChildNode));

				if (nodeName == "ShortName")
				{
					// Ignore LongName, use this as long name
					properties.m_longName = nodeContent;
				}
				else if (nodeName == "Url")
				{
					ustring url, type;
					SearchPluginProperties::Response response = SearchPluginProperties::RSS_RESPONSE;
					bool getMethod = true;

					// Parse Query Syntax
					Element::AttributeList attributes = pElem->get_attributes();
					for (Element::AttributeList::const_iterator iter = attributes.begin();
						iter != attributes.end(); ++iter)
					{
						Attribute *pAttr = (*iter);

						if (pAttr != NULL)
						{
							ustring attrName = pAttr->get_name();
							ustring attrContent = pAttr->get_value();

							if (attrName == "template")
							{
								url = attrContent;
							}
							else if (attrName == "type")
							{
								type = attrContent;
							}
							else if (attrName == "method")
							{
								// GET is the default method
								if (StringManip::toLowerCase(attrContent) != "get")
								{
									getMethod = false;
								}
							}
						}
					}

					// Did we get the URL ?
					if (url.empty() == true)
					{
						// It's probably provided as content, v1.0 style
						url = nodeContent;
					}

					if (getMethod == true)
					{
						string::size_type startPos = 0, pos = url.find("?");

						// Do we support that type ?
						if (type == "application/atom+xml")
						{
							response = SearchPluginProperties::ATOM_RESPONSE;
							rssResponse = false;
						}
						else if ((type.empty() == false) &&
							(type != "application/rss+xml"))
						{
							response = SearchPluginProperties::UNKNOWN_RESPONSE;
#ifdef DEBUG
							clog << "OpenSearchParser::parse: unsupported response type "
								<< type << endl;
#endif
							continue;
						}
	
						// Break the URL down into base and parameters
						if (pos != string::npos)
						{
							string params(url.substr(pos + 1));

							// URL
							properties.m_baseUrl = url.substr(0, pos);
#ifdef DEBUG
							clog << "OpenSearchParser::parse: URL is " << url << endl;
#endif

							// Split this into the actual parameters
							params += "&";
							pos = params.find("&");
							while (pos != string::npos)
							{
								string parameter(params.substr(startPos, pos - startPos));

								string::size_type equalPos = parameter.find("=");
								if (equalPos != string::npos)
								{
									string paramName(parameter.substr(0, equalPos));
									string paramValue(parameter.substr(equalPos + 1));
									SearchPluginProperties::ParameterVariable param = SearchPluginProperties::UNKNOWN_PARAM;

									if (paramValue == "{searchTerms}")
									{
										param = SearchPluginProperties::SEARCH_TERMS_PARAM;
									}
									else if (paramValue == "{count}")
									{
										param = SearchPluginProperties::COUNT_PARAM;
									}
									else if (paramValue == "{startIndex}")
									{
										param = SearchPluginProperties::START_INDEX_PARAM;
									}
									else if (paramValue == "{startPage}")
									{
										param = SearchPluginProperties::START_PAGE_PARAM;
									}
									else if (paramValue == "{language}")
									{
										param = SearchPluginProperties::LANGUAGE_PARAM;
									}
									else if (paramValue == "{outputEncoding}")
									{
										param = SearchPluginProperties::OUTPUT_ENCODING_PARAM;
									}
									else if (paramValue == "{inputEncoding}")
									{
										param = SearchPluginProperties::INPUT_ENCODING_PARAM;
									}

									if (param != SearchPluginProperties::UNKNOWN_PARAM)
									{
										properties.m_variableParameters[param] = paramName;
									}
									else
									{
#ifdef DEBUG
										clog << "OpenSearchParser::parse: " << paramName << "=" << paramValue << endl;
#endif
										if (paramValue.substr(0, 5) == "EDIT:")
										{
											// This is user editable
											properties.m_editableParameters[paramName] = paramValue.substr(5);
										}
										else
										{
											// Append to the remainder
											if (properties.m_remainder.empty() == false)
											{
												properties.m_remainder += "&";
											}
											properties.m_remainder += paramName;
											properties.m_remainder += "=";
											properties.m_remainder += paramValue;
										}
									}
								}

								// Next
								startPos = pos + 1;
								pos = params.find_first_of("&", startPos);
							}
						}

						// Method
						properties.m_method = SearchPluginProperties::GET_METHOD;
						// Output type
						properties.m_outputType = type;
						// Response
						properties.m_response = response;
					}

					// We ignore Param as we only support GET
				}
				else if (nodeName == "Tags")
				{
					// This is supposed to be a space-delimited list, but use the whole thing as channel
					properties.m_channel = nodeContent;
				}
				else if (nodeName == "Language")
				{
					properties.m_languages.insert(nodeContent);
				}
			}
		}
	}
	catch (const std::exception& ex)
	{
#ifdef DEBUG
		clog << "OpenSearchParser::parse: caught exception: " << ex.what() << endl;
#endif
		success = false;
	}

	if (success == false)
	{
		return NULL;
	}

	// Scrolling
	properties.m_nextBase = 1;
	if (properties.m_variableParameters.find(SearchPluginProperties::START_PAGE_PARAM) != properties.m_variableParameters.end())
	{
		properties.m_scrolling = SearchPluginProperties::PER_PAGE;
		properties.m_nextIncrement = 1;
	}
	else if ((properties.m_variableParameters.find(SearchPluginProperties::COUNT_PARAM) != properties.m_variableParameters.end()) ||
		(properties.m_variableParameters.find(SearchPluginProperties::START_INDEX_PARAM) != properties.m_variableParameters.end()))
	{
		properties.m_scrolling = SearchPluginProperties::PER_INDEX;
		properties.m_nextIncrement = 0;
	}
	else
	{
		// No scrolling
		properties.m_nextIncrement = 0;
		properties.m_nextBase = 0;
	}

	return new OpenSearchResponseParser(rssResponse);
}
