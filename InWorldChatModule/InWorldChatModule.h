/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   InWorldChatModule.h
 *  @brief  Simple OpenSim world chat module. Listens for ChatFromSimulator packets and shows the chat on the UI.
 *          Outgoing chat sent using ChatFromViewer packets. Manages EC_ChatBubbles
 */

#ifndef incl_InWorldChatModule_InWorldChatModule_h
#define incl_InWorldChatModule_InWorldChatModule_h

#include "ModuleInterface.h"
#include "ModuleLoggingFunctions.h"
#include "WorldStream.h"
#include "ConsoleCommandServiceInterface.h"

#include <QObject>

namespace Foundation
{
    class EventDataInterface;
}

namespace ProtocolUtilities
{
    class ProtocolModuleInterface;
    typedef boost::weak_ptr<ProtocolModuleInterface> ProtocolWeakPtr;
}

namespace UiServices
{
    class UiModule;
}

namespace Naali
{
    class InWorldChatModule :  public QObject, public Foundation::ModuleInterfaceImpl
    {
        Q_OBJECT

    public:
        /// Default constructor.
        InWorldChatModule();

        /// Destructor 
        ~InWorldChatModule();

        /// ModuleInterfaceImpl overrides.
        void Load();
        void PostInitialize();
        void Update(f64 frametime);
        bool HandleEvent(
            event_category_id_t category_id,
            event_id_t event_id,
            Foundation::EventDataInterface* data);

        void SubscribeToNetworkEvents(ProtocolUtilities::ProtocolWeakPtr currentProtocolModule);

        MODULE_LOGGING_FUNCTIONS

        /// Returns name of this module. Needed for logging.
        static const std::string &NameStatic();

        /// Name of this module.
        static const std::string moduleName;

        Console::CommandResult TestAddBillboard(const StringVector &params);

        Console::CommandResult ConsoleChat(const StringVector &params);

    public slots:
        ///
        /// @param msg Chat message to be sent.
        void SendChatFromViewer(const QString &msg);

    private:
        Q_DISABLE_COPY(InWorldChatModule);

        /// NetworkState event category.
        event_category_id_t networkStateEventCategory_;

        /// NetworkIn event category.
        event_category_id_t networkInEventCategory_;

        /// Framework event category
        event_category_id_t frameworkEventCategory_;

        /// WorldStream pointer
        ProtocolUtilities::WorldStreamPtr currentWorldStream_ ;

        /// UiModule pointer.
        boost::weak_ptr<UiServices::UiModule> uiModule_;
    };
}

#endif
