/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 * @brief Manages Content Apps
 */

#pragma once

#include <app-common/zap-generated/enums.h>
#include <app/app-platform/ContentApp.h>
#include <app/app-platform/ContentAppPlatform.h>
#include <app/util/attribute-storage.h>
#include <functional>
#include <jni.h>
#include <lib/core/DataModelTypes.h>
#include <lib/support/JniReferences.h>
#include <stdbool.h>
#include <stdint.h>

#include "../include/account-login/AccountLoginManager.h"
#include "../include/application-basic/ApplicationBasicManager.h"
#include "../include/application-launcher/ApplicationLauncherManager.h"
#include "../include/content-launcher/AppContentLauncherManager.h"
#include "../include/media-playback/AppMediaPlaybackManager.h"
#include "../include/target-navigator/TargetNavigatorManager.h"
#include "ChannelManager.h"
#include "CommissionerMain.h"
#include "ContentAppAttributeDelegate.h"
#include "KeypadInputManager.h"
#include <app/clusters/account-login-server/account-login-delegate.h>
#include <app/clusters/application-basic-server/application-basic-delegate.h>
#include <app/clusters/application-launcher-server/application-launcher-delegate.h>
#include <app/clusters/channel-server/channel-delegate.h>
#include <app/clusters/content-launch-server/content-launch-delegate.h>
#include <app/clusters/keypad-input-server/keypad-input-delegate.h>
#include <app/clusters/media-playback-server/media-playback-delegate.h>
#include <app/clusters/target-navigator-server/target-navigator-delegate.h>

CHIP_ERROR InitVideoPlayerPlatform(jobject contentAppEndpointManager);
EndpointId AddContentApp(const char * szVendorName, uint16_t vendorId, const char * szApplicationName, uint16_t productId,
                         const char * szApplicationVersion, jobject manager);
EndpointId AddContentApp(const char * szVendorName, uint16_t vendorId, const char * szApplicationName, uint16_t productId,
                         const char * szApplicationVersion, EndpointId endpointId, jobject manager);
EndpointId RemoveContentApp(EndpointId epId);
void ReportAttributeChange(EndpointId epId, chip::ClusterId clusterId, chip::AttributeId attributeId);

#if CHIP_DEVICE_CONFIG_APP_PLATFORM_ENABLED

namespace chip {
namespace AppPlatform {

using AccountLoginDelegate        = app::Clusters::AccountLogin::Delegate;
using ApplicationBasicDelegate    = app::Clusters::ApplicationBasic::Delegate;
using ApplicationLauncherDelegate = app::Clusters::ApplicationLauncher::Delegate;
using ChannelDelegate             = app::Clusters::Channel::Delegate;
using ContentLauncherDelegate     = app::Clusters::ContentLauncher::Delegate;
using KeypadInputDelegate         = app::Clusters::KeypadInput::Delegate;
using MediaPlaybackDelegate       = app::Clusters::MediaPlayback::Delegate;
using TargetNavigatorDelegate     = app::Clusters::TargetNavigator::Delegate;
using SupportedStreamingProtocol  = app::Clusters::ContentLauncher::SupportedStreamingProtocol;
using ContentAppAttributeDelegate = chip::AppPlatform::ContentAppAttributeDelegate;

static const int kCatalogVendorId = CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID;

// for this platform, appid is just vendor id
#define BuildAppId(vid) std::to_string(vid).c_str()

class DLL_EXPORT ContentAppImpl : public ContentApp
{
public:
    ContentAppImpl(const char * szVendorName, uint16_t vendorId, const char * szApplicationName, uint16_t productId,
                   const char * szApplicationVersion, const char * setupPIN, ContentAppAttributeDelegate * attributeDelegate) :
        mApplicationBasicDelegate(kCatalogVendorId, BuildAppId(vendorId), szVendorName, vendorId, szApplicationName, productId,
                                  szApplicationVersion),
        mAccountLoginDelegate(setupPIN), mContentLauncherDelegate(attributeDelegate, { "image/*", "video/*" },
                                                                  to_underlying(SupportedStreamingProtocol::kDash) |
                                                                      to_underlying(SupportedStreamingProtocol::kHls)),
        mMediaPlaybackDelegate(attributeDelegate), mTargetNavigatorDelegate({ "home", "search", "info", "guide", "menu" }, 0){};
    virtual ~ContentAppImpl() {}

    AccountLoginDelegate * GetAccountLoginDelegate() override { return &mAccountLoginDelegate; };
    ApplicationBasicDelegate * GetApplicationBasicDelegate() override { return &mApplicationBasicDelegate; };
    ApplicationLauncherDelegate * GetApplicationLauncherDelegate() override { return &mApplicationLauncherDelegate; };
    ChannelDelegate * GetChannelDelegate() override { return &mChannelDelegate; };
    ContentLauncherDelegate * GetContentLauncherDelegate() override
    {
        mContentLauncherDelegate.SetEndpointId(GetEndpointId());
        return &mContentLauncherDelegate;
    };
    KeypadInputDelegate * GetKeypadInputDelegate() override { return &mKeypadInputDelegate; };
    MediaPlaybackDelegate * GetMediaPlaybackDelegate() override
    {
        mMediaPlaybackDelegate.SetEndpointId(GetEndpointId());
        return &mMediaPlaybackDelegate;
    };
    TargetNavigatorDelegate * GetTargetNavigatorDelegate() override { return &mTargetNavigatorDelegate; };

protected:
    ApplicationBasicManager mApplicationBasicDelegate;
    AccountLoginManager mAccountLoginDelegate;
    ApplicationLauncherManager mApplicationLauncherDelegate;
    ChannelManager mChannelDelegate;
    AppContentLauncherManager mContentLauncherDelegate;
    KeypadInputManager mKeypadInputDelegate;
    AppMediaPlaybackManager mMediaPlaybackDelegate;
    TargetNavigatorManager mTargetNavigatorDelegate;
};

class DLL_EXPORT ContentAppFactoryImpl : public ContentAppFactory
{
#define APP_LIBRARY_SIZE 4
public:
    ContentAppFactoryImpl();
    virtual ~ContentAppFactoryImpl() {}

    // Lookup CatalogVendor App for this client (vendor id/product id client)
    // and then write it to destinationApp
    // return error if not found
    CHIP_ERROR LookupCatalogVendorApp(uint16_t vendorId, uint16_t productId, CatalogVendorApp * destinationApp) override;

    // Lookup ContentApp for this catalog id / app id and load it
    ContentApp * LoadContentApp(const CatalogVendorApp & vendorApp) override;

    EndpointId AddContentApp(const char * szVendorName, uint16_t vendorId, const char * szApplicationName, uint16_t productId,
                             const char * szApplicationVersion, jobject manager);

    EndpointId AddContentApp(const char * szVendorName, uint16_t vendorId, const char * szApplicationName, uint16_t productId,
                             const char * szApplicationVersion, jobject manager, EndpointId desiredEndpointId);

    EndpointId RemoveContentApp(EndpointId epId);

    // Gets the catalog vendor ID used by this platform
    uint16_t GetPlatformCatalogVendorId() override;

    // Converts application (any catalog) into the platform's catalog Vendor
    // and then writes it to destinationApp
    CHIP_ERROR ConvertToPlatformCatalogVendorApp(const CatalogVendorApp & sourceApp, CatalogVendorApp * destinationApp) override;

    // Get the privilege this vendorId should have on endpoints 1, 2, and content app endpoints
    // In the case of casting video clients, this should usually be Access::Privilege::kOperate
    // and for voice agents, this may be Access::Privilege::kAdminister
    // When a vendor has admin privileges, it will get access to all clusters on ep1
    Access::Privilege GetVendorPrivilege(uint16_t vendorId) override;

    void AddAdminVendorId(uint16_t vendorId);

    void setContentAppAttributeDelegate(ContentAppAttributeDelegate * attributeDelegate);

protected:
    std::vector<ContentAppImpl *> mContentApps{
        new ContentAppImpl("Vendor1", 1, "exampleid", 11, "Version1", "20202021", nullptr),
        new ContentAppImpl("Vendor2", 65521, "exampleString", 32768, "Version2", "20202021", nullptr),
        new ContentAppImpl("Vendor3", 9050, "App3", 22, "Version3", "20202021", nullptr),
        new ContentAppImpl("TestSuiteVendor", 1111, "applicationId", 22, "v2", "20202021", nullptr)
    };
    std::vector<DataVersion *> mDataVersions{};

    std::vector<uint16_t> mAdminVendorIds{};

private:
    ContentAppAttributeDelegate * mAttributeDelegate;
};

} // namespace AppPlatform
} // namespace chip

chip::AppPlatform::ContentAppFactoryImpl * GetContentAppFactoryImpl();

#endif // CHIP_DEVICE_CONFIG_APP_PLATFORM_ENABLED
