#include "StableHeaders.h"
#include "AssetModule.h"
#include "AssetInterface.h"
#include "OpenSimProtocolModule.h"
#include "NetInMessage.h"
#include "RexProtocolMsgIDs.h"
#include "AssetDefines.h"
#include "AssetManager.h"
#include "AssetTransfer.h"

using namespace OpenSimProtocol;
using namespace RexTypes;

namespace Asset
{
    const char *AssetManager::DEFAULT_ASSET_CACHE_PATH = "/assetcache";
    const Core::f64 AssetManager::DEFAULT_ASSET_TIMEOUT = 60.0;

    AssetManager::AssetManager(Foundation::Framework* framework, OpenSimProtocolModule* net_interface) : 
        framework_(framework),
        net_interface_(net_interface),
        asset_timeout_(DEFAULT_ASSET_TIMEOUT)
    {
        // Create asset cache directory
        cache_path_ = framework_->GetPlatform()->GetApplicationDataDirectory() + DEFAULT_ASSET_CACHE_PATH;
        if (boost::filesystem::exists(cache_path_) == false)
        {
            boost::filesystem::create_directory(cache_path_);
        }
    }
    
    AssetManager::~AssetManager()
    {
    }
    
    Foundation::AssetPtr AssetManager::GetAsset(const std::string& asset_id)
    {
        Foundation::AssetPtr no_asset;
        return no_asset;
    }
    
    void AssetManager::Update(Core::f64 frametime)
    {
        AssetTransferMap::iterator i = texture_transfers_.begin();
        while (i != texture_transfers_.end())
        {
            AssetTransfer& transfer = i->second;
            if (!transfer.Ready())
            {
                transfer.AddTime(frametime);
                if (transfer.GetTime() > asset_timeout_)
                {
                    AssetModule::LogInfo("Texture transfer " + transfer.GetAssetId().ToString() + " timed out.");
                    if (net_interface_)
                    {
                        // Send cancel message
                        const ClientParameters& client = net_interface_->GetClientParameters();
                        NetOutMessage *m = net_interface_->StartMessageBuilding(RexNetMsgRequestImage);
                        assert(m);
                        
                        m->AddUUID(client.agentID);
                        m->AddUUID(client.sessionID);
        
                        m->SetVariableBlockCount(1);
                        m->AddUUID(transfer.GetAssetId()); // Image UUID
                        m->AddS8(-1); // Discard level, -1 = cancel
                        m->AddF32(0.0); // Download priority, 0 = cancel
                        m->AddU32(0); // Starting packet
                        m->AddU8(RexIT_Normal); // Image type
        
                        net_interface_->FinishMessageBuilding(m);
                    }
                    
                    texture_transfers_.erase(i);
                }
            }
            ++i;
        }
        
        AssetTransferMap::iterator j = asset_transfers_.begin();
        while (j != asset_transfers_.end())
        {
            AssetTransfer& transfer = j->second;
            if (!transfer.Ready())
            {
                transfer.AddTime(frametime);
                if (transfer.GetTime() > asset_timeout_)
                {
                    AssetModule::LogInfo("Asset transfer " + transfer.GetAssetId().ToString() + " timed out.");
                    if (net_interface_)
                    {
                        // Send cancel message
                        NetOutMessage *m = net_interface_->StartMessageBuilding(RexNetMsgTransferAbort);
                        assert(m);
                        m->AddUUID(j->first); // Transfer ID
                        m->AddS32(RexAC_Asset); // Asset channel type
                        net_interface_->FinishMessageBuilding(m);
                    }
                    
                    asset_transfers_.erase(j);
                }
            }
            ++j;
        }
    }
    
    void AssetManager::RequestAsset(const RexUUID& asset_id, Core::uint asset_type)
    {
        if (assets_.find(asset_id) != assets_.end())
        {
            AssetModule::LogInfo("Asset " + asset_id.ToString() + " already received");
            return;
        }

        GetFromCache(asset_id);
        
        if (assets_.find(asset_id) != assets_.end())
        {
            AssetModule::LogInfo("Asset " + asset_id.ToString() + " already received");
            return;
        }
        
        if (!net_interface_)
        {
            AssetModule::LogError("No netinterface, cannot request assets");
            return;
        }
        
        if (asset_type == RexAT_Texture)
        {
            RequestTexture(asset_id);
        }
        else
        {
            RequestOtherAsset(asset_id, asset_type);
        }
    }
    
    void AssetManager::RequestTexture(const RexUUID& asset_id)
    {
        if (!net_interface_)
            return;
        
        const ClientParameters& client = net_interface_->GetClientParameters();
        
        if (texture_transfers_.find(asset_id) != texture_transfers_.end())
        {
            AssetModule::LogWarning("Texture " + asset_id.ToString() + " already requested");
            return;
        }
        
        AssetTransfer new_transfer;
        new_transfer.SetAssetId(asset_id);
        new_transfer.SetAssetType(RexAT_Texture);
        texture_transfers_[asset_id] = new_transfer;
    
        AssetModule::LogInfo("Requesting texture " + asset_id.ToString());

        NetOutMessage *m = net_interface_->StartMessageBuilding(RexNetMsgRequestImage);
        assert(m);
        
        m->AddUUID(client.agentID);
        m->AddUUID(client.sessionID);
        
        m->SetVariableBlockCount(1);
        m->AddUUID(asset_id); // Image UUID
        m->AddS8(0); // Discard level
        m->AddF32(100.0); // Download priority
        m->AddU32(0); // Starting packet
        m->AddU8(RexIT_Normal); // Image type
        
        net_interface_->FinishMessageBuilding(m);
    }
    
    void AssetManager::RequestOtherAsset(const RexUUID& asset_id, Core::uint asset_type)
    {
        // Asset transfers are keyed by transfer id, not asset id, so have to search in a bit cumbersome way
        AssetTransferMap::const_iterator i = asset_transfers_.begin();
        while (i != asset_transfers_.end())
        {
            if (i->second.GetAssetId() == asset_id)
            {
                AssetModule::LogWarning("Asset " + asset_id.ToString() + " already requested");
                return;
            }
            ++i;
        }

        RexUUID transfer_id;
        transfer_id.Random();
        
        AssetTransfer new_transfer;
        new_transfer.SetAssetId(asset_id);
        new_transfer.SetAssetType(asset_type);
        asset_transfers_[transfer_id] = new_transfer;
        
        AssetModule::LogInfo("Requesting asset " + asset_id.ToString());
        
        NetOutMessage *m = net_interface_->StartMessageBuilding(RexNetMsgTransferRequest);
        assert(m);
        
        m->AddUUID(transfer_id); // Transfer ID
        m->AddS32(RexAC_Asset); // Asset channel type
        m->AddS32(RexAS_Asset); // Asset source type
        m->AddF32(100.0); // Download priority
        
        Core::u8 asset_info[20]; // Asset info block with UUID and type
        memcpy(&asset_info[0], &asset_id.data, 16);
        memcpy(&asset_info[16], &asset_type, 4);
        m->AddBuffer(20, asset_info);
        
        net_interface_->FinishMessageBuilding(m);
    }
    
    void AssetManager::HandleTextureHeader(NetInMessage* msg)
    {
        RexUUID asset_id = msg->ReadUUID();
        AssetTransferMap::iterator i = texture_transfers_.find(asset_id);
        if (i == texture_transfers_.end())
        {
            AssetModule::LogWarning("Data received for nonexisting texture transfer " + asset_id.ToString());
            return;
        }
        
        AssetTransfer& transfer = i->second;
        
        Core::u8 codec = msg->ReadU8();
        Core::u32 size = msg->ReadU32();
        Core::u16 packets = msg->ReadU16();
        
        transfer.SetSize(size);
        
        Core::uint data_size; 
        const Core::u8* data = msg->ReadBuffer(&data_size); // ImageData block
        transfer.ReceiveData(0, data, data_size);
        
        //AssetModule::LogInfo("First packet received for " + asset_id.ToString() + ", " + Core::ToString<Core::u16>(data_size) + " bytes");
        
        if (transfer.Ready())
        {
            StoreAsset(transfer);
            texture_transfers_.erase(i);
        }
    }
    
    void AssetManager::HandleTextureData(NetInMessage* msg)
    {
        RexUUID asset_id = msg->ReadUUID();
        AssetTransferMap::iterator i = texture_transfers_.find(asset_id);
        if (i == texture_transfers_.end())
        {
            AssetModule::LogWarning("Data received for nonexisting texture transfer " + asset_id.ToString());
            return;
        }
        
        AssetTransfer& transfer = i->second;
        
        Core::u16 packet_index = msg->ReadU16();
        
        Core::uint data_size; 
        const Core::u8* data = msg->ReadBuffer(&data_size); // ImageData block
        transfer.ReceiveData(packet_index, data, data_size);
        
        //AssetModule::LogInfo("Packet " + Core::ToString<Core::u16>(packet_index) + " received for " + asset_id.ToString() + ", " + Core::ToString<Core::u16>(data_size) + " bytes");
        
        if (transfer.Ready())
        {
            StoreAsset(transfer);
            texture_transfers_.erase(i);
        }
    }
    
    void AssetManager::HandleTextureCancel(NetInMessage* msg)
    {
        RexUUID asset_id = msg->ReadUUID();
        AssetTransferMap::iterator i = texture_transfers_.find(asset_id);
        if (i == texture_transfers_.end())
        {
            AssetModule::LogWarning("Cancel received for nonexisting texture transfer " + asset_id.ToString());
            return;
        }

        AssetModule::LogInfo("Transfer of texture " + asset_id.ToString() + " canceled");
        texture_transfers_.erase(i);
    }
    
    void AssetManager::HandleAssetHeader(NetInMessage* msg)
    {
        RexUUID transfer_id = msg->ReadUUID();
        AssetTransferMap::iterator i = asset_transfers_.find(transfer_id);
        if (i == asset_transfers_.end())
        {
            AssetModule::LogWarning("Data received for nonexisting asset transfer " + transfer_id.ToString());
            return;
        }
        
        AssetTransfer& transfer = i->second;
        
        Core::s32 channel_type = msg->ReadS32();
        Core::s32 target_type = msg->ReadS32();
        Core::s32 status = msg->ReadS32();
        Core::s32 size = msg->ReadS32();
        
        if ((status != RexTS_Ok) && (status != RexTS_Done))
        {
            AssetModule::LogInfo("Transfer for asset " + transfer.GetAssetId().ToString() + " canceled with code " + Core::ToString<Core::s32>(status));
            asset_transfers_.erase(i);
            return;
        }
        
        transfer.SetSize(size);
        
        // We may get data packets before header, so check if all already received
        if (transfer.Ready())
        {
            StoreAsset(transfer);
            asset_transfers_.erase(i);
        }
    }
    
    void AssetManager::HandleAssetData(NetInMessage* msg)
    {
        RexUUID transfer_id = msg->ReadUUID();
        AssetTransferMap::iterator i = asset_transfers_.find(transfer_id);
        if (i == asset_transfers_.end())
        {
            AssetModule::LogWarning("Data received for nonexisting asset transfer " + transfer_id.ToString());
            return;
        }
        
        AssetTransfer& transfer = i->second;
        
        Core::s32 channel_type = msg->ReadS32();
        Core::s32 packet_index = msg->ReadS32();
        Core::s32 status = msg->ReadS32();
        
        if ((status != RexTS_Ok) && (status != RexTS_Done))
        {
            AssetModule::LogInfo("Transfer for asset " + transfer.GetAssetId().ToString() + " canceled with code " + Core::ToString<Core::s32>(status));
            asset_transfers_.erase(i);
            return;
        }
        
        Core::uint data_size; 
        const Core::u8* data = msg->ReadBuffer(&data_size); // Data block
        transfer.ReceiveData(packet_index, data, data_size);
        
        //AssetModule::LogInfo("Packet " + Core::ToString<Core::u16>(packet_index) + " received for " + transfer.GetAssetId().ToString() + ", " + Core::ToString<Core::u16>(data_size) + " bytes");
        
        if (transfer.Ready())
        {
            StoreAsset(transfer);
            asset_transfers_.erase(i);
        }
    }
    
    void AssetManager::HandleAssetCancel(NetInMessage* msg)
    {
        RexUUID transfer_id = msg->ReadUUID();
        AssetTransferMap::iterator i = asset_transfers_.find(transfer_id);
        if (i == asset_transfers_.end())
        {
            AssetModule::LogWarning("Cancel received for nonexisting asset transfer " + transfer_id.ToString());
            return;
        }
        
        AssetModule::LogInfo("Transfer for asset " + i->second.GetAssetId().ToString() + " canceled");
        asset_transfers_.erase(i);
    }
    
    void AssetManager::StoreAsset(AssetTransfer& transfer)
    {
        const RexUUID& asset_id = transfer.GetAssetId();
        
        AssetModule::LogInfo("Storing complete asset " + asset_id.ToString());

        assets_[asset_id].asset_type_ = transfer.GetAssetType();
        assets_[asset_id].data_.resize(transfer.GetReceived());
        transfer.AssembleData(&assets_[asset_id].data_[0]);
        
        boost::filesystem::path file_path(cache_path_ + "/" + asset_id.ToString());
        
        std::ofstream filestr(file_path.native_directory_string().c_str(), std::ios::out | std::ios::binary);
        if (filestr.good())
        {
            Core::uint type = transfer.GetAssetType();
            // Store first the asset type, then the actual data
            filestr.write((const char *)&type, sizeof(type));
            filestr.write((const char *)&assets_[asset_id].data_[0], assets_[asset_id].data_.size());
            filestr.close();
        }
        else
        {
            AssetModule::LogError("Error storing asset " + asset_id.ToString() + " to cache.");
        }
    }
    
    void AssetManager::GetFromCache(const RexTypes::RexUUID& asset_id)
    {
        boost::filesystem::path file_path(cache_path_ + "/" + asset_id.ToString());
        
        std::ifstream filestr(file_path.native_directory_string().c_str(), std::ios::in | std::ios::binary);
        if (filestr.good())
        {
            filestr.seekg(0, std::ios::end);
            Core::uint length = filestr.tellg();
            filestr.seekg(0, std::ios::beg);

            Core::uint type;
            if (length > sizeof(type))
            {
                length -= sizeof(type);
                
                filestr.read((char *)&type, sizeof(type));
                assets_[asset_id].asset_type_ = type;
                assets_[asset_id].data_.resize(length);
                filestr.read((char *)&assets_[asset_id].data_[0], length);
            }
            else
            {
                AssetModule::LogError("Malformed asset file " + asset_id.ToString() + " found in cache.");
            }
            
            filestr.close();
        }
    }
}