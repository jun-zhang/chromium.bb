/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
    Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#ifndef ResourceFetcher_h
#define ResourceFetcher_h

#include "core/fetch/CachePolicy.h"
#include "core/fetch/FetchInitiatorInfo.h"
#include "core/fetch/FetchRequest.h"
#include "core/fetch/Resource.h"
#include "core/fetch/ResourceLoaderHost.h"
#include "core/fetch/ResourceLoaderOptions.h"
#include "core/fetch/ResourcePtr.h"
#include "platform/Timer.h"
#include "wtf/Deque.h"
#include "wtf/HashMap.h"
#include "wtf/HashSet.h"
#include "wtf/ListHashSet.h"
#include "wtf/text/StringHash.h"

namespace blink {

class ArchiveResourceCollection;
class CSSStyleSheetResource;
class DocumentResource;
class FetchContext;
class FontResource;
class ImageResource;
class MHTMLArchive;
class RawResource;
class ScriptResource;
class SubstituteData;
class XSLStyleSheetResource;
class Document;
class DocumentLoader;
class LocalFrame;
class ImageLoader;
class KURL;
class ResourceTimingInfo;
class ResourceLoaderSet;

// The ResourceFetcher provides a per-context interface to the MemoryCache
// and enforces a bunch of security checks and rules for resource revalidation.
// Its lifetime is roughly per-DocumentLoader, in that it is generally created
// in the DocumentLoader constructor and loses its ability to generate network
// requests when the DocumentLoader is destroyed. Documents also hold a
// RefPtr<ResourceFetcher> for their lifetime (and will create one if they
// are initialized without a LocalFrame), so a Document can keep a ResourceFetcher
// alive past detach if scripts still reference the Document.
class ResourceFetcher final : public RefCountedWillBeGarbageCollectedFinalized<ResourceFetcher>, public ResourceLoaderHost {
    WTF_MAKE_NONCOPYABLE(ResourceFetcher); WTF_MAKE_FAST_ALLOCATED_WILL_BE_REMOVED;
    WILL_BE_USING_GARBAGE_COLLECTED_MIXIN(ResourceFetcher);
friend class ImageLoader;
friend class ResourceCacheValidationSuppressor;

public:
    static PassRefPtrWillBeRawPtr<ResourceFetcher> create(DocumentLoader* documentLoader) { return adoptRefWillBeNoop(new ResourceFetcher(documentLoader)); }
    virtual ~ResourceFetcher();
    DECLARE_VIRTUAL_TRACE();

#if !ENABLE(OILPAN)
    using RefCounted<ResourceFetcher>::ref;
    using RefCounted<ResourceFetcher>::deref;
#endif

    ResourcePtr<Resource> fetchSynchronously(FetchRequest&);
    ResourcePtr<ImageResource> fetchImage(FetchRequest&);
    ResourcePtr<CSSStyleSheetResource> fetchCSSStyleSheet(FetchRequest&);
    ResourcePtr<ScriptResource> fetchScript(FetchRequest&);
    ResourcePtr<FontResource> fetchFont(FetchRequest&);
    ResourcePtr<RawResource> fetchRawResource(FetchRequest&);
    ResourcePtr<RawResource> fetchMainResource(FetchRequest&, const SubstituteData&);
    ResourcePtr<DocumentResource> fetchSVGDocument(FetchRequest&);
    ResourcePtr<XSLStyleSheetResource> fetchXSLStyleSheet(FetchRequest&);
    ResourcePtr<Resource> fetchLinkResource(Resource::Type, FetchRequest&);
    ResourcePtr<RawResource> fetchImport(FetchRequest&);
    ResourcePtr<RawResource> fetchMedia(FetchRequest&);
    ResourcePtr<RawResource> fetchTextTrack(FetchRequest&);

    // Logs an access denied message to the console for the specified URL.
    void printAccessDeniedMessage(const KURL&) const;

    Resource* cachedResource(const KURL&) const;

    typedef HashMap<String, ResourcePtr<Resource>> DocumentResourceMap;
    const DocumentResourceMap& allResources() const { return m_documentResources; }

    bool autoLoadImages() const { return m_autoLoadImages; }
    void setAutoLoadImages(bool);

    void setImagesEnabled(bool);

    bool shouldDeferImageLoad(const KURL&) const;

    LocalFrame* frame() const; // Can be null
    FetchContext& context() const;
    Document* document() const { return m_document; } // Can be null
    void setDocument(RawPtr<Document> document) { m_document = document; }

    DocumentLoader* documentLoader() const { return m_documentLoader; }
    void clearDocumentLoader() { m_documentLoader = nullptr; }

    void garbageCollectDocumentResources();

    int requestCount() const;

    bool isPreloaded(const String& urlString) const;
    void clearPreloads();
    void preload(Resource::Type, FetchRequest&, const String& charset);
    void printPreloadStats();

    void addAllArchiveResources(MHTMLArchive*);
    ArchiveResourceCollection* archiveResourceCollection() const { return m_archiveResourceCollection.get(); }

    void setDefersLoading(bool);
    void stopFetching();
    bool isFetching() const;

    // ResourceLoaderHost
    virtual void didLoadResource() override;
    virtual void redirectReceived(Resource*, const ResourceResponse&) override;
    virtual void didFinishLoading(Resource*, double finishTime, int64_t encodedDataLength) override;
    virtual void didChangeLoadingPriority(const Resource*, ResourceLoadPriority, int intraPriorityValue) override;
    virtual void didFailLoading(const Resource*, const ResourceError&) override;
    virtual void willSendRequest(unsigned long identifier, ResourceRequest&, const ResourceResponse& redirectResponse, const FetchInitiatorInfo&) override;
    virtual void didReceiveResponse(const Resource*, const ResourceResponse&) override;
    virtual void didReceiveData(const Resource*, const char* data, int dataLength, int encodedDataLength) override;
    virtual void didDownloadData(const Resource*, int dataLength, int encodedDataLength) override;
    virtual void subresourceLoaderFinishedLoadingOnePart(ResourceLoader*) override;
    virtual void didInitializeResourceLoader(ResourceLoader*) override;
    virtual void willStartLoadingResource(Resource*, ResourceRequest&) override;
    virtual bool defersLoading() const override;
    virtual bool isLoadedBy(ResourceLoaderHost*) const override;
    virtual bool canAccessRedirect(Resource*, ResourceRequest&, const ResourceResponse&, ResourceLoaderOptions&) override;
    virtual bool canAccessResource(Resource*, SecurityOrigin*, const KURL&) const override;
    virtual bool isControlledByServiceWorker() const override;

#if !ENABLE(OILPAN)
    virtual void refResourceLoaderHost() override;
    virtual void derefResourceLoaderHost() override;
#endif

    int64_t serviceWorkerID() const;
    void acceptDataFromThreadedReceiver(unsigned long identifier, const char* data, int dataLength, int encodedDataLength);

    enum ResourceLoadStartType {
        ResourceLoadingFromNetwork,
        ResourceLoadingFromCache
    };
    void maybeNotifyInsecureContent(const Resource*) const;
    void requestLoadStarted(Resource*, const FetchRequest&, ResourceLoadStartType);
    static const ResourceLoaderOptions& defaultResourceOptions();

    String getCacheIdentifier() const;

    virtual ResourceLoaderHost::LoaderHostType objectType() const override { return ResourceFetcherType; };

    static ResourceFetcher* toResourceFetcher(ResourceLoaderHost*);

private:
    friend class ResourceFetcherTest;

    explicit ResourceFetcher(DocumentLoader*);

    bool shouldLoadNewResource(Resource::Type) const;

    ResourcePtr<Resource> requestResource(Resource::Type, FetchRequest&);
    ResourcePtr<Resource> createResourceForRevalidation(const FetchRequest&, Resource*);
    ResourcePtr<Resource> createResourceForLoading(Resource::Type, FetchRequest&, const String& charset);
    void preCacheDataURIImage(const FetchRequest&);
    void preCacheSubstituteDataForMainResource(const FetchRequest&, const SubstituteData&);
    void storeResourceTimingInitiatorInformation(Resource*);
    bool scheduleArchiveLoad(Resource*, const ResourceRequest&);

    enum RevalidationPolicy { Use, Revalidate, Reload, Load };
    RevalidationPolicy determineRevalidationPolicy(Resource::Type, const FetchRequest&, Resource* existingResource) const;

    void determineRequestContext(ResourceRequest&, Resource::Type);
    ResourceRequestCachePolicy resourceRequestCachePolicy(const ResourceRequest&, Resource::Type);
    void addAdditionalRequestHeaders(ResourceRequest&, Resource::Type);
    void upgradeInsecureRequest(FetchRequest&);

    bool canRequest(Resource::Type, const ResourceRequest&, const KURL&, const ResourceLoaderOptions&, bool forPreload, FetchRequest::OriginRestriction) const;

    static bool resourceNeedsLoad(Resource*, const FetchRequest&, RevalidationPolicy);

    void notifyLoadedFromMemoryCache(Resource*);

    void garbageCollectDocumentResourcesTimerFired(Timer<ResourceFetcher>*);
    void scheduleDocumentResourcesGC();

    void resourceTimingReportTimerFired(Timer<ResourceFetcher>*);

    bool clientDefersImage(const KURL&) const;
    void reloadImagesIfNotDeferred();

    void willTerminateResourceLoader(ResourceLoader*);

    HashSet<String> m_validatedURLs;
    mutable DocumentResourceMap m_documentResources;
    // FIXME: Oilpan: Ideally this should just be a traced Member but that will
    // currently leak because LayoutStyle and its data are not on the heap.
    // See crbug.com/383860 for details.
    RawPtrWillBeWeakMember<Document> m_document;
    DocumentLoader* m_documentLoader;

    OwnPtr<ListHashSet<Resource*>> m_preloads;
    OwnPtrWillBeMember<ArchiveResourceCollection> m_archiveResourceCollection;

    Timer<ResourceFetcher> m_garbageCollectDocumentResourcesTimer;
    Timer<ResourceFetcher> m_resourceTimingReportTimer;

    typedef HashMap<Resource*, RefPtr<ResourceTimingInfo>> ResourceTimingInfoMap;
    ResourceTimingInfoMap m_resourceTimingInfoMap;

    HashMap<RefPtr<ResourceTimingInfo>, bool> m_scheduledResourceTimingReports;

    OwnPtrWillBeMember<ResourceLoaderSet> m_loaders;
    OwnPtrWillBeMember<ResourceLoaderSet> m_nonBlockingLoaders;

    // Used in hit rate histograms.
    class DeadResourceStatsRecorder {
    public:
        DeadResourceStatsRecorder();
        ~DeadResourceStatsRecorder();

        void update(RevalidationPolicy);

    private:
        int m_useCount;
        int m_revalidateCount;
        int m_loadCount;
    };
    DeadResourceStatsRecorder m_deadStatsRecorder;

    // 29 bits left
    bool m_autoLoadImages : 1;
    bool m_imagesEnabled : 1;
    bool m_allowStaleResources : 1;
};

class ResourceCacheValidationSuppressor {
    WTF_MAKE_NONCOPYABLE(ResourceCacheValidationSuppressor);
    WTF_MAKE_FAST_ALLOCATED;
public:
    ResourceCacheValidationSuppressor(ResourceFetcher* loader)
        : m_loader(loader)
        , m_previousState(false)
    {
        if (m_loader) {
            m_previousState = m_loader->m_allowStaleResources;
            m_loader->m_allowStaleResources = true;
        }
    }
    ~ResourceCacheValidationSuppressor()
    {
        if (m_loader)
            m_loader->m_allowStaleResources = m_previousState;
    }
private:
    ResourceFetcher* m_loader;
    bool m_previousState;
};

} // namespace blink

#endif
