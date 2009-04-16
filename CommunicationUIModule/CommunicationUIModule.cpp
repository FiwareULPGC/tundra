#pragma warning( push )
#pragma warning( disable : 4250 )
#undef max
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/actiongroup.h>

#include <libglademm.h>
#include <glade/glade.h>
#pragma warning( pop )

#include <Poco/ClassLibrary.h>
//#include <sstream>

#include "StableHeaders.h"
#include "Foundation.h"
#include "ConfigureDlg.h"
#include "ScriptServiceInterface.h"

#include "CommunicationUIModule.h"

//#include "PythonScriptModule.h"


void testCallback(char*);

namespace Communication
{

	CommunicationUIModule::CommunicationUIModule(void):ModuleInterfaceImpl("CommunicationUIModule")
	{
	}

	CommunicationUIModule::~CommunicationUIModule(void)
	{
	}

	void CommunicationUIModule::Load()
	{
	}
	
	void CommunicationUIModule::Unload()
	{
	}

	void CommunicationUIModule::Initialize() 
	{
	}

	void CommunicationUIModule::PostInitialize()
	{
		initializeMainCommWindow();
		commManager = framework_->GetService<Foundation::Comms::CommunicationManagerServiceInterface>(Foundation::Service::ST_CommunicationManager);
		scriptService = framework_->GetService<Foundation::ScriptServiceInterface>(Foundation::Service::ST_Scripting);
		//std::string error;
		//Foundation::ScriptObject* script = scriptService->LoadScript("IMDemo", error);
		//if(error=="None")
		//{
		//	this->imScriptObject = script->GetObject("IMDemo");
		//	std::string str = "DoStartUp";
		//	std::string syntax = "";
		//	Foundation::ScriptObject* ret = imScriptObject->CallMethod(str, syntax, NULL);
		//}
	}

	void CommunicationUIModule::Uninitialize()
	{
	}

	void CommunicationUIModule::Update()
	{
	}

	void CommunicationUIModule::initializeMainCommWindow()
	{
		commUI_XML = Gnome::Glade::Xml::create("data/communicationUI.glade");
		if (!commUI_XML)
			return;
		wndCommMain = 0;
		                        
		commUI_XML->get_widget("wndCMain", wndCommMain);
		if(!wndCommMain)
			return;

	    commUI_XML->connect_clicked("mi_connect", sigc::mem_fun(*this, &CommunicationUIModule::OnAccountMenuConnect));
		commUI_XML->connect_clicked("mi_disconnect", sigc::mem_fun(*this, &CommunicationUIModule::OnAccountMenuDisconnect));
		commUI_XML->connect_clicked("mi_setaccount", sigc::mem_fun(*this, &CommunicationUIModule::OnAccountMenuSetAccountAndPassword));
		commUI_XML->connect_clicked("mi_settings", sigc::mem_fun(*this, &CommunicationUIModule::OnAccountMenuSettings));
		commUI_XML->connect_clicked("btnTest", sigc::mem_fun(*this, &CommunicationUIModule::OnAccountMenuConnect));

		wndCommMain->show();
	}

	void CommunicationUIModule::OnAccountMenuSettings()
	{
		LogInfo("something clicked");
		std::string error;
		
		//Foundation::ScriptObject* script = scriptService->LoadScript("pymodules/py_class3", error);
		Foundation::ScriptObject* script = scriptService->LoadScript("py_class3", error);
		
		if(error=="None"){
			LogInfo("got script object");
			
			sobj = script->GetObject("Multiply");
			std::string str = "multiply";
			std::string syntax = "";
			Foundation::ScriptObject* ret = sobj->CallMethod(str, syntax, NULL);
			LogInfo("Received: ");
			char* cret = ret->ConvertToChar();
			if(cret!=NULL){
				LogInfo(cret);
			} else {
				LogInfo("NULL");
			}

		} else {
			LogInfo("script loading failed");
		}
		//scriptService->RunString("Test.Run()");
	}

	void CommunicationUIModule::OnAccountMenuSetAccountAndPassword()
	{
		LogInfo("Set account");
		std::map<std::string, Foundation::Comms::SettingsAttribute> attributes = commManager->GetAccountAttributes();
		int count = attributes.size();
		LogInfo(attributes.find("name")->first);

		//ConfigureDlg accDlg(count, attributes, "account settings", commManager, this);
		ConfigureDlg accDlg(count, attributes, "account settings", this);
		Gtk::Main::run(accDlg);
		//accDlg.
	}

	void CommunicationUIModule::OnAccountMenuConnect()
	{
		LogInfo("something clicked");
		//commManager->Connect();		
		std::string meth = "TestCallback2";
		std::string syntax = "s";
		char** carr = new char*[2];
		char* buf1 = new char[10];
		strcpy(buf1,"blob");
		carr[0] = buf1;



		
		sobj->CallMethod(meth, syntax, carr);


		//scriptService->RunScript("Test");
		

		//PythonScript::PythonScriptModule *pyModule_ = dynamic_cast<PythonScript::PythonScriptModule *>(framework_->GetModuleManager()->GetModule(Foundation::Module::MT_PythonScript));
		//pyModule_->DoConnect();
	}
	
	void CommunicationUIModule::OnAccountMenuDisconnect()
	{
		LogInfo("something clicked");
		//scriptService->RunString("Test.SetTrue()");
		//sobj->PassFunctionPointerToScript(&testCallback, "setCallback2", "key");
		
		Foundation::ScriptEventInterface *eIntf = dynamic_cast<Foundation::ScriptEventInterface*>(this->scriptService);
		//Foundation::ScriptEventInterface *eIntf = (Foundation::ScriptEventInterface*)scriptService;
		
		eIntf->SetCallback(CommunicationUIModule::testCallback, "key");
		
		//commManager->Disconnect();	
	}

	void CommunicationUIModule::Callback(std::string aConfigName, std::map<std::string, Foundation::Comms::SettingsAttribute> attributes)
	{
		if(aConfigName=="account settings"){ commManager->SetAccountAttributes(attributes); }
	}

	void CommunicationUIModule::testCallback(char* t)
	{
		std::cout << "testCallback" << std::endl;
		std::cout << t << std::endl;
	}

}

void testCallback(char*)
{
	std::cout << "in testCallback" << std::endl;
}

using namespace Communication;

POCO_BEGIN_MANIFEST(Foundation::ModuleInterface)
POCO_EXPORT_CLASS(CommunicationUIModule)
POCO_END_MANIFEST
