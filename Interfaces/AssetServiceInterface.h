// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_Interfaces_AssetServiceInterface_h
#define incl_Interfaces_AssetServiceInterface_h

#include "ServiceInterface.h"

namespace Foundation
{
    class AssetInterface;
    typedef boost::shared_ptr<AssetInterface> AssetPtr;
    
    /*! An asset container service implements this interface to provide client modules
        with asset request and retrieval functionality.
        See \ref AssetModule for details on how to use the asset service.
     */
    class AssetServiceInterface : public ServiceInterface
    {
    public:
        AssetServiceInterface() {}
        virtual ~AssetServiceInterface() {}

        //! Gets asset
        /*! If asset not in cache, will return empty pointer 
            Does not queue an asset download request.

            \param asset_id Asset ID, UUID for legacy UDP assets
            \param asset_type Asset type
            \return Pointer to asset
            
         */
        virtual AssetPtr GetAsset(const std::string& asset_id, Core::asset_type_t asset_type) = 0;
        
        //! Gets incomplete asset
        /*! If not enough bytes received, will return empty pointer
            Does not queue an asset download request.
            
            \param asset_id Asset ID, UUID for legacy UDP assets
            \param asset_type Asset type
            \param received Minimum continuous bytes received from the start
            \return Pointer to asset
         */
        virtual AssetPtr GetIncompleteAsset(const std::string& asset_id, Core::asset_type_t asset_type, Core::uint received) = 0;
        
        //! Requests an asset download
        /*! If asset already downloaded, does nothing.
            Events will be sent when download progresses, and when asset is ready.

            \param asset_id Asset ID, UUID for legacy UDP assets
            \param asset_type Asset type
         */
        virtual void RequestAsset(const std::string& asset_id, Core::asset_type_t asset_type) = 0;

        //! Queries status of asset download
        /*! If asset has been already fully received, size, received & received_continuous will be the same
        
            \param asset_id Asset ID, UUID for legacy UDP assets
            \param size Variable to receive asset size (if known, 0 if unknown)
            \param received Variable to receive amount of bytes received
            \param received_continuous Variable to receive amount of continuous bytes received from the start
            \return true If asset was found either in cache or as a transfer in progress, and variables have been filled, false if not found
         */
        virtual bool QueryAssetStatus(const std::string& asset_id, Core::uint& size, Core::uint& received, Core::uint& received_continuous) = 0;
    };
}

#endif
