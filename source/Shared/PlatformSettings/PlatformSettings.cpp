#include "Shared/PlatformSettings.h"
#include "OPS_Core/Serialization.h"
#include "OPS_Core/IO.h"
#include "OPS_Core/Kernel.h"

namespace OPS
{
namespace Shared
{
	using namespace OPS::Reprise;

	PlatformSettings::PlatformSettings(const std::wstring pathToFile) 
	{
		if (!LoadFromFileNew(pathToFile))
			if (!LoadFromFileOld(pathToFile))
				throw RuntimeError("Failed to load platform settings from file XML.");
	}
	
	bool PlatformSettings::LoadFromFileNew(const std::wstring PathToFile)
	{
		using namespace Serialization;
		Schema root(L"Root", Schema::MODE_REQUIRED_UNIQUE);
		root.addParameter(L"BitPlatform", ParameterInfo(Parameter::PARAMETER_TYPE_INT, ParameterInfo::MODE_REQUIRED_UNIQUE));
		root.addParameter(L"CallProc", ParameterInfo(Parameter::PARAMETER_TYPE_LONG_INT, ParameterInfo::MODE_REQUIRED_UNIQUE));

		//Type
		Schema& typeSchema = root.addSchema(L"Type", Schema::MODE_NORMAL);
		typeSchema.addParameter(L"Kind", ParameterInfo(Parameter::PARAMETER_TYPE_INT, ParameterInfo::MODE_REQUIRED_UNIQUE));
		typeSchema.addParameter(L"Name", ParameterInfo(Parameter::PARAMETER_TYPE_STRING, ParameterInfo::MODE_REQUIRED_UNIQUE));
		//Item
		Schema& itemSchema = typeSchema.addSchema(L"Item", Schema::MODE_NORMAL);
		itemSchema.addParameter(L"Kind", ParameterInfo(Parameter::PARAMETER_TYPE_INT, ParameterInfo::MODE_REQUIRED_UNIQUE));
		itemSchema.addParameter(L"Name", ParameterInfo(Parameter::PARAMETER_TYPE_STRING, ParameterInfo::MODE_REQUIRED_UNIQUE));
		itemSchema.addParameter(L"Class", ParameterInfo(Parameter::PARAMETER_TYPE_STRING, ParameterInfo::MODE_REQUIRED_UNIQUE));
		itemSchema.addParameter(L"Complexity", ParameterInfo(Parameter::PARAMETER_TYPE_LONG_INT, ParameterInfo::MODE_REQUIRED_UNIQUE));
		itemSchema.addParameter(L"Size", ParameterInfo(Parameter::PARAMETER_TYPE_LONG_INT, ParameterInfo::MODE_REQUIRED_UNIQUE));
		
		XMLSerializer ser(root);
		try
		{
			long_long_t ValueCall=0;
			std::unique_ptr<Item> item(ser.read(IO::combinePath(getStartupPath(), PathToFile)));			
			//Root
			if (item->getName()==L"Root")				
			{
				if (item->hasParameter(L"BitPlatform"))
					m_BitPlatform = item->getParameter(L"BitPlatform").getInt();
				if (item->hasParameter(L"CallProc"))
					ValueCall = item->getParameter(L"CallProc").getLongInt();
			}
			//Type
			Item::items_type::iterator type_iter = item->begin();
			for (;type_iter!=item->end() ; type_iter++)
			{		
				Item& itemType = (*type_iter);
				if ((itemType.getName()==L"Type") && itemType.hasParameter(L"Kind"))
				{					
					int iKindType = itemType.getParameter(L"Kind").getInt();				
					//Operation
					MapHardwareOptionsForCallKind mapHOfCK;
					Item::items_type::iterator oper_iter = itemType.begin();
					for (;oper_iter!=itemType.end() ; oper_iter++)
					{
						Item& itemOper = (*oper_iter);
						if (itemOper.getName()==L"Item" && itemOper.hasParameter(L"Kind") && itemOper.hasParameter(L"Class") &&
							  itemOper.hasParameter(L"Complexity") && itemOper.hasParameter(L"Size"))
						{
							int iKindOper = itemOper.getParameter(L"Kind").getInt();							
							HardwareOptions HO;
							HO.Complexity = itemOper.getParameter(L"Complexity").getLongInt();
							HO.SizeCommand = itemOper.getParameter(L"Size").getLongInt();
							if((iKindOper==(int)OCK_READ) || 
								 (iKindOper==(int)BasicCallExpression::BCK_ASSIGN) ||
								 (iKindOper==(int)OCK_ARGUMENT))
								HO.SizeData = GetSizeForType((BasicType::BasicTypes)iKindType);
							else
								HO.SizeData = 0;
							mapHOfCK[iKindOper]=HO;								
						}
					}
					m_TabHardwareOptions[(BasicType::BasicTypes)iKindType]=mapHOfCK;		
				}
			}
			//Сложность вызова подпрограммы
			HardwareOptions HOCall;
			HOCall.Complexity = ValueCall;
			MapHardwareOptionsForCallKind mapHOfCKCall;
			mapHOfCKCall[OCK_CALL]=HOCall;
			m_TabHardwareOptions[BasicType::BT_UNDEFINED]=mapHOfCKCall;

			//Для VOID скопировать с INT
			m_TabHardwareOptions[BasicType::BT_VOID]=m_TabHardwareOptions[GetKindForPtrType()];
		}
		catch(const OPS::Exception)
		{
			return false; //throw RuntimeError("Failed to load platform settings from file XML.");  
		}		
		return true;
	}

	bool PlatformSettings::LoadFromFileOld(const std::wstring PathToFile)
	{
		using namespace Serialization;
		Schema root(L"Root", Schema::MODE_REQUIRED_UNIQUE);
		root.addParameter(L"BitPlatform", ParameterInfo(Parameter::PARAMETER_TYPE_INT, ParameterInfo::MODE_REQUIRED_UNIQUE));
		root.addParameter(L"CallProc", ParameterInfo(Parameter::PARAMETER_TYPE_LONG_INT, ParameterInfo::MODE_REQUIRED_UNIQUE));

		//Type
		Schema& typeSchema = root.addSchema(L"Type", Schema::MODE_NORMAL);
		typeSchema.addParameter(L"Kind", ParameterInfo(Parameter::PARAMETER_TYPE_INT, ParameterInfo::MODE_REQUIRED_UNIQUE));
		typeSchema.addParameter(L"Name", ParameterInfo(Parameter::PARAMETER_TYPE_STRING, ParameterInfo::MODE_REQUIRED_UNIQUE));
		//Item
		Schema& itemSchema = typeSchema.addSchema(L"Item", Schema::MODE_NORMAL);
		itemSchema.addParameter(L"Kind", ParameterInfo(Parameter::PARAMETER_TYPE_INT, ParameterInfo::MODE_REQUIRED_UNIQUE));
		itemSchema.addParameter(L"Name", ParameterInfo(Parameter::PARAMETER_TYPE_STRING, ParameterInfo::MODE_REQUIRED_UNIQUE));
		itemSchema.addParameter(L"Class", ParameterInfo(Parameter::PARAMETER_TYPE_STRING, ParameterInfo::MODE_REQUIRED_UNIQUE));
		itemSchema.addParameter(L"Value", ParameterInfo(Parameter::PARAMETER_TYPE_LONG_INT, ParameterInfo::MODE_REQUIRED_UNIQUE));
		
		XMLSerializer ser(root);
		try
		{
			long_long_t ValueCall=0;
			std::unique_ptr<Item> item(ser.read(IO::combinePath(getStartupPath(), PathToFile)));			
			//Root
			if (item->getName()==L"Root")				
			{
				if (item->hasParameter(L"BitPlatform"))
					m_BitPlatform = item->getParameter(L"BitPlatform").getInt();
				if (item->hasParameter(L"CallProc"))
					ValueCall = item->getParameter(L"CallProc").getLongInt();
			}
			//Type
			Item::items_type::iterator type_iter = item->begin();
			for (;type_iter!=item->end() ; type_iter++)
			{		
				Item& itemType = (*type_iter);
				if ((itemType.getName()==L"Type") && itemType.hasParameter(L"Kind"))
				{					
					int iKindType = itemType.getParameter(L"Kind").getInt();				
					//Operation
					MapHardwareOptionsForCallKind mapHOfCK;
					Item::items_type::iterator oper_iter = itemType.begin();
					for (;oper_iter!=itemType.end() ; oper_iter++)
					{
						Item& itemOper = (*oper_iter);
						if((itemOper.getName()==L"Item") && itemOper.hasParameter(L"Kind") && 
							  itemOper.hasParameter(L"Class") && itemOper.hasParameter(L"Value"))
						{
							int iKindOper = itemOper.getParameter(L"Kind").getInt();							
							std::wstring wsClass = itemOper.getParameter(L"Class").getString();
							if (wsClass==L"OtherCallKind")
								iKindOper+=100;
							HardwareOptions HO;							
							HO.Complexity = itemOper.getParameter(L"Value").getLongInt();
							HO.SizeCommand = 0;
							if((iKindOper==(int)OCK_READ) || 
								 (iKindOper==(int)BasicCallExpression::BCK_ASSIGN) ||
								 (iKindOper==(int)OCK_ARGUMENT))
								HO.SizeData = GetSizeForType((BasicType::BasicTypes)iKindType);
							else
								HO.SizeData = 0;
							mapHOfCK[iKindOper]=HO;									
						}
					}
					m_TabHardwareOptions[(BasicType::BasicTypes)iKindType]=mapHOfCK;		
				}
			}
			//Сложность вызова подпрограммы
			HardwareOptions HOCall;
			HOCall.Complexity = ValueCall;
			MapHardwareOptionsForCallKind mapHOfCKCall;
			mapHOfCKCall[OCK_CALL]=HOCall;
			m_TabHardwareOptions[BasicType::BT_UNDEFINED]=mapHOfCKCall;

			//Для VOID скопировать с INT
			m_TabHardwareOptions[BasicType::BT_VOID]=m_TabHardwareOptions[GetKindForPtrType()];
		}
		catch(const OPS::Exception)
		{
			return false; //throw RuntimeError("Failed to load platform settings from file XML.");  
		}		
		return true;
	}	

	HardwareOptions PlatformSettings::GetValue(int callKind, OPS::Reprise::BasicType::BasicTypes typeKind)
	{
		if (m_TabHardwareOptions.find(typeKind) != m_TabHardwareOptions.end())
		{
			MapHardwareOptionsForCallKind &mapHOfCK = m_TabHardwareOptions[typeKind];
			if (mapHOfCK.find(callKind)!= mapHOfCK.end())
				return mapHOfCK[callKind];
		}
		HardwareOptions HO;
		return HO;
	}

	BasicType::BasicTypes PlatformSettings::GetKindForPtrType()
	{
		switch (m_BitPlatform)
		{
			case  8: return BasicType::BT_INT8;
			case 16: return BasicType::BT_INT16;
			case 32: return BasicType::BT_INT32;
			case 64: return BasicType::BT_INT64;
            case 128: return BasicType::BT_INT128;
			default: return BasicType::BT_INT32;
		}
	}

	long_long_t PlatformSettings::GetSizeForType(BasicType::BasicTypes type)
	{
		switch (type)
		{
			case BasicType::BT_INT8 : return 1;
			case BasicType::BT_INT16: return 2;
			case BasicType::BT_INT32: return 4;
			case BasicType::BT_INT64: return 8;
            case BasicType::BT_INT128: return 16;
			default: return 4;
		}
	}

}
}
