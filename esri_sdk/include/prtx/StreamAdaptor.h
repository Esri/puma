/*
  COPYRIGHT (c) 2012-2020 Esri R&D Center Zurich
  TRADE SECRETS: ESRI PROPRIETARY AND CONFIDENTIAL
  Unpublished material - all rights reserved under the
  Copyright Laws of the United States and applicable international
  laws, treaties, and conventions.

  For additional information, contact:
  Environmental Systems Research Institute, Inc.
  Attn: Contracts and Legal Services Department
  380 New York Street
  Redlands, California, 92373
  USA

  email: contracts@esri.com
*/

#ifndef PRTX_STREAM_ADAPTOR_H_
#define PRTX_STREAM_ADAPTOR_H_

#include "prt/ContentType.h"
#include "prt/Cache.h"

#include "prtx/prtx.h"
#include "prtx/Content.h"
#include "prtx/URI.h"
#include "prtx/Extension.h"
#include "prtx/ResolveMap.h"

#include <string>
#include <iosfwd>
#include <memory>


namespace prtx {

class StreamAdaptor;
using StreamAdaptorPtr = std::shared_ptr<StreamAdaptor>;

/**
 * Base class for byte stream adaptors. Subclasses implement how a byte stream is extracted based
 * from a certain kind of URIs. Typically, there is more or less one stream adaptor for each supported URI scheme.
 *
 * \sa prtx::URI
 */
class PRTX_EXPORTS_API StreamAdaptor : public Extension {
protected:
	StreamAdaptor() = default;

public:
	virtual ~StreamAdaptor() = default;

	/**
	 * The URI specified by resolveMap and corresponding key is read and decoded
	 * into prtx::Content based object(s) using the stream create by createStream.
	 *
	 * \sa prt::ResolveMap
	 */
	virtual void resolve(
			ContentPtrVector&		results,	///< Receives the decoded objects.
			prt::Cache*				cache,		///< Cache for nested calls to prtx::DataBackend.
			const std::wstring&		key,		///< Resource key into resolve map.
			prt::ContentType		ct,			///< Only decoders with ContentType ct will be tried.
			prtx::ResolveMap const*	resolveMap,	///< The available URIs.
			std::wstring&			warnings	///< Receives any resolve/decode warnings generated by the call.
	) const final;

	/**
	 * Implements the logic how to create a byte stream for a specific URI.
	 *
	 * @return Returns a new instance of std::istream or subclass. Must be destroyed with destroyStream.
	 */
	virtual std::istream* createStream(prtx::URIPtr uri) const = 0;

	/**
	 * Destroys the stream created by createStream. The stream must be created with createStream.
	 */
	virtual void destroyStream(std::istream* stream) const = 0;

	/**
	 * Fixates the extension type to ET_STREAM_ADAPTOR.
	 *
	 * @return Always returns prtx::Extension::ET_STREAM_ADAPTOR.
	 *
	 * \sa prtx::Extension
	 */
	virtual prtx::Extension::ExtensionType getExtensionType() const final override {
		return Extension::ET_STREAM_ADAPTOR;
	}

	/**
	 * The content type of a Stream Adpator is undefined, it can decode into multiple possible content objects.
	 *
	 * @return Always returns prt::CT_UNDEFINED.
	 *
	 * \sa prtx::Content
	 */
	virtual prt::ContentType getContentType() const final override {
		return prt::CT_UNDEFINED;
	}
};


} // namespace prtx


#endif /* PRTX_STREAM_ADAPTOR_H_ */
