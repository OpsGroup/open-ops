#include "MPIHelperDeepWalkers.h"

#include "Transforms/MPI/Utils/MPIHelper.h"
#include "Transforms/MPI/Utils/MPIProducerFactory.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/SubroutinesShared.h"

namespace OPS
{
    namespace Transforms
    {
        namespace MPIProducer
        {
            using namespace std;
            using namespace OPS::Reprise;
            using namespace OPS::Shared;
            using namespace OPS::Shared::ExpressionHelpers;

            /*
                MPIFunctionsHelper implementation
            */

            MPIFunctionsHelper::MPIFunctionsHelper(): m_mpiFunctionsNames()
            {
            }

            string MPIFunctionsHelper::getFunctionName(MPI_FUNCS mpiFunctionKind) const
            {
                OPS_ASSERT(m_mpiFunctionsNames.find(mpiFunctionKind) != m_mpiFunctionsNames.end());

                return m_mpiFunctionsNames.find(mpiFunctionKind)->second;
            }

            MPIFunctionsHelper::~MPIFunctionsHelper()
            {
            }

            
            /*
                MPIFunctionsCHelper implementation
            */
            
            MPIFunctionsCHelper::MPIFunctionsCHelper(): MPIFunctionsHelper()
            {
                fillMPIFunctionsNames();
            }

            
            MPIFunctionsCHelper* MPIFunctionsCHelper::ms_pInstanceObject = NULL;

            MPIFunctionsCHelper& MPIFunctionsCHelper::getInstance()
            {
                if(ms_pInstanceObject == NULL)
                {
                    ms_pInstanceObject = new MPIFunctionsCHelper();
                }

                return *ms_pInstanceObject;
            }

            
            void MPIFunctionsCHelper::fillMPIFunctionsNames()
            {
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_INIT]        = "MPI_Init";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_FINALIZE]    = "MPI_Finalize";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_COMM_RANK]   = "MPI_Comm_rank";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_COMM_SIZE]   = "MPI_Comm_size";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_TYPE_VECTOR] = "MPI_Type_vector";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_TYPE_HVECTOR] = "MPI_Type_hvector";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_TYPE_EXTENT] = "MPI_Type_extent";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_TYPE_COMMIT] = "MPI_Type_commit";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_TYPE_FREE]   = "MPI_Type_free";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_SEND]        = "MPI_Send";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_ISEND]       = "MPI_Isend";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_WAIT]        = "MPI_Wait";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_RECV]        = "MPI_Recv";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_IRECV]       = "MPI_Irecv";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_ALLGATHER]   = "MPI_Allgather";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_BCAST]       = "MPI_Bcast";
            }

            
            /*
                MPIFunctionsFHelper implementation
            */
            
            MPIFunctionsFHelper::MPIFunctionsFHelper(): MPIFunctionsHelper()
            {
                fillMPIFunctionsNames();
            }

            
            MPIFunctionsFHelper* MPIFunctionsFHelper::ms_pInstanceObject = NULL;

            MPIFunctionsFHelper& MPIFunctionsFHelper::getInstance()
            {
                if(ms_pInstanceObject == NULL)
                {
                    ms_pInstanceObject = new MPIFunctionsFHelper();
                }

                return *ms_pInstanceObject;
            }

            
            void MPIFunctionsFHelper::fillMPIFunctionsNames()
            {
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_INIT]        = "MPI_INIT";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_FINALIZE]    = "MPI_FINALIZE";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_COMM_RANK]   = "MPI_COMM_RANK";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_COMM_SIZE]   = "MPI_COMM_SIZE";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_TYPE_VECTOR] = "MPI_TYPE_VECTOR";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_TYPE_VECTOR] = "MPI_TYPE_HVECTOR";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_TYPE_EXTENT] = "MPI_TYPE_EXTENT";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_TYPE_COMMIT] = "MPI_TYPE_COMMIT";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_TYPE_FREE]   = "MPI_TYPE_FREE";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_SEND]        = "MPI_SEND";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_RECV]        = "MPI_RECV";
                m_mpiFunctionsNames[MPIFunctionsCHelper::MF_MPI_ALLGATHER]   = "MPI_ALLGATHER";
            }


            
            /*
                MPIBasicTypesHelper implementation
            */

            MPIBasicTypesHelper::MPIBasicTypesHelper(): m_mpiBasicTypesNames()
            {
            }

            std::string MPIBasicTypesHelper::getMPIBasicTypeName(MPI_BASIC_TYPES mpiBasicTypeKind) const
            {
                OPS_ASSERT(m_mpiBasicTypesNames.find(mpiBasicTypeKind) != m_mpiBasicTypesNames.end());

                return m_mpiBasicTypesNames.find(mpiBasicTypeKind)->second;
            }
                
            MPIBasicTypesHelper::MPI_BASIC_TYPES MPIBasicTypesHelper::getMPIBasicTypeByRepriseBasicType(OPS::Reprise::BasicType::BasicTypes repriseBasicTypeKind) const
            {
                OPS_ASSERT(m_reprise2MPITipesMap.find(repriseBasicTypeKind) != m_reprise2MPITipesMap.end());

                return m_reprise2MPITipesMap.find(repriseBasicTypeKind)->second;
            }

            MPIBasicTypesHelper::~MPIBasicTypesHelper()
            {
            }
    
            /*
                MPIBasicTypesCHelper implementation
            */
            
            MPIBasicTypesCHelper::MPIBasicTypesCHelper(): MPIBasicTypesHelper()
            {
                fillMPIBasicTypesNames();
                fillReprise2MPITipesMap();
            }

            
            MPIBasicTypesCHelper* MPIBasicTypesCHelper::ms_pInstanceObject = NULL;

            void MPIBasicTypesCHelper::fillMPIBasicTypesNames()
            {
                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_INTEGER1] = "MPI_INTEGER1";
                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_INTEGER2] = "MPI_INTEGER2";
                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_INTEGER4] = "MPI_INTEGER4";
                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_INTEGER8] = "MPI_INTEGER8";

                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_REAL4]    = "MPI_REAL4";
                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_REAL8]    = "MPI_REAL8";

                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_CHAR]    = "MPI_CHAR";
            }

            void MPIBasicTypesCHelper::fillReprise2MPITipesMap()
            {
                m_reprise2MPITipesMap[BasicType::BT_INT8] = MPIBasicTypesHelper::MBT_MPI_INTEGER1;
                m_reprise2MPITipesMap[BasicType::BT_INT16] = MPIBasicTypesHelper::MBT_MPI_INTEGER2;
                m_reprise2MPITipesMap[BasicType::BT_INT32] = MPIBasicTypesHelper::MBT_MPI_INTEGER4;
                m_reprise2MPITipesMap[BasicType::BT_INT64] = MPIBasicTypesHelper::MBT_MPI_INTEGER8;

                m_reprise2MPITipesMap[BasicType::BT_FLOAT32] = MPIBasicTypesHelper::MBT_MPI_REAL4;
                m_reprise2MPITipesMap[BasicType::BT_FLOAT64] = MPIBasicTypesHelper::MBT_MPI_REAL8;

                m_reprise2MPITipesMap[BasicType::BT_CHAR] = MPIBasicTypesHelper::MBT_MPI_CHAR;
            }
            
            MPIBasicTypesCHelper& MPIBasicTypesCHelper::getInstance()
            {
                if(ms_pInstanceObject == NULL)
                {
                    ms_pInstanceObject = new MPIBasicTypesCHelper();
                }

                return *ms_pInstanceObject;
            }

            
            /*
                MPIBasicTypesFHelper implementation
            */
            
            MPIBasicTypesFHelper::MPIBasicTypesFHelper(): MPIBasicTypesHelper()
            {
                fillMPIBasicTypesNames();
            }

            
            MPIBasicTypesFHelper* MPIBasicTypesFHelper::ms_pInstanceObject = NULL;

            void MPIBasicTypesFHelper::fillMPIBasicTypesNames()
            {
                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_INTEGER1] = "MPI_INTEGER1";
                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_INTEGER2] = "MPI_INTEGER2";
                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_INTEGER4] = "MPI_INTEGER4";
                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_INTEGER8] = "MPI_INTEGER8";

                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_REAL4]    = "MPI_REAL4";
                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_REAL8]    = "MPI_REAL8";

                m_mpiBasicTypesNames[MPIBasicTypesHelper::MBT_MPI_CHAR]    = "MPI_CHAR";
            }
            
            void MPIBasicTypesFHelper::fillReprise2MPITipesMap()
            {
                m_reprise2MPITipesMap[BasicType::BT_INT8] = MPIBasicTypesHelper::MBT_MPI_INTEGER1;
                m_reprise2MPITipesMap[BasicType::BT_INT16] = MPIBasicTypesHelper::MBT_MPI_INTEGER2;
                m_reprise2MPITipesMap[BasicType::BT_INT32] = MPIBasicTypesHelper::MBT_MPI_INTEGER4;
                m_reprise2MPITipesMap[BasicType::BT_INT64] = MPIBasicTypesHelper::MBT_MPI_INTEGER8;

                m_reprise2MPITipesMap[BasicType::BT_FLOAT32] = MPIBasicTypesHelper::MBT_MPI_REAL4;
                m_reprise2MPITipesMap[BasicType::BT_FLOAT64] = MPIBasicTypesHelper::MBT_MPI_REAL8;

                m_reprise2MPITipesMap[BasicType::BT_CHAR] = MPIBasicTypesHelper::MBT_MPI_CHAR;
            }
            
            MPIBasicTypesFHelper& MPIBasicTypesFHelper::getInstance()
            {
                if(ms_pInstanceObject == NULL)
                {
                    ms_pInstanceObject = new MPIBasicTypesFHelper();
                }

                return *ms_pInstanceObject;
            }


            
            /*
                MPITypesHelper implementation
            */

            MPITypesHelper::MPITypesHelper(): m_mpiTypesNames()
            {
            }

            std::string MPITypesHelper::getMPITypeName(MPI_TYPES mpiTypeKind) const
            {
                OPS_ASSERT(m_mpiTypesNames.find(mpiTypeKind) != m_mpiTypesNames.end());

                return m_mpiTypesNames.find(mpiTypeKind)->second;
            }
            
            MPITypesHelper::~MPITypesHelper()
            {
            }
            
            /*
                MPITypesCHelper implementation
            */
            
            MPITypesCHelper::MPITypesCHelper(): MPITypesHelper()
            {
                fillMPITypesNames();
            }

            
            MPITypesCHelper* MPITypesCHelper::ms_pInstanceObject = NULL;

            void MPITypesCHelper::fillMPITypesNames()
            {
                m_mpiTypesNames[MPITypesHelper::MT_MPI_DATATYPE] = "MPI_Datatype";
                m_mpiTypesNames[MPITypesHelper::MT_MPI_COMM]     = "MPI_Comm";
                m_mpiTypesNames[MPITypesHelper::MT_MPI_STATUS]   = "MPI_Status";
                m_mpiTypesNames[MPITypesHelper::MT_MPI_REQUEST]  = "MPI_Request";
            }

            
            MPITypesCHelper& MPITypesCHelper::getInstance()
            {
                if(ms_pInstanceObject == NULL)
                {
                    ms_pInstanceObject = new MPITypesCHelper();
                }

                return *ms_pInstanceObject;
            }

            
            /*
                MPITypesFHelper implementation. Fortran version of MPI does not contain special data types - all are integer
            */
            
            MPITypesFHelper::MPITypesFHelper(): MPITypesHelper()
            {
                fillMPITypesNames();
            }

            
            MPITypesFHelper* MPITypesFHelper::ms_pInstanceObject = NULL;

            void MPITypesFHelper::fillMPITypesNames()
            {
                m_mpiTypesNames[MPITypesHelper::MT_MPI_DATATYPE] = "DUMMY";
                m_mpiTypesNames[MPITypesHelper::MT_MPI_COMM]     = "DUMMY";
                m_mpiTypesNames[MPITypesHelper::MT_MPI_STATUS]   = "DUMMY";
            }

            
            MPITypesFHelper& MPITypesFHelper::getInstance()
            {
                if(ms_pInstanceObject == NULL)
                {
                    ms_pInstanceObject = new MPITypesFHelper();
                }

                return *ms_pInstanceObject;
            }

            /*
                MPICommsHelper implementation
            */

            MPICommsHelper::MPICommsHelper(): m_mpiCommsNames()
            {
                
            }

            MPICommsHelper::~MPICommsHelper()
            {
            }

            std::string MPICommsHelper::getMPICommName( MPI_COMMS mpiCommKind ) const
            {
                OPS_ASSERT(m_mpiCommsNames.find(mpiCommKind) != m_mpiCommsNames.end());

                return m_mpiCommsNames.find(mpiCommKind)->second;
            }
            
            /*
                MPICommsCHelper implementation
            */
            
            MPICommsCHelper::MPICommsCHelper(): MPICommsHelper()
            {
                fillMPICommsNames();
            }

            
            MPICommsCHelper* MPICommsCHelper::ms_pInstanceObject = NULL;

            void MPICommsCHelper::fillMPICommsNames()
            {
                m_mpiCommsNames[MPICommsHelper::MC_MPI_COMM_WORLD] = "MPI_COMM_WORLD";
                m_mpiCommsNames[MPICommsHelper::MC_MPI_COMM_SELF]  = "MPI_COMM_SELF";
            }

            
            MPICommsCHelper& MPICommsCHelper::getInstance()
            {
                if(ms_pInstanceObject == NULL)
                {
                    ms_pInstanceObject = new MPICommsCHelper();
                }

                return *ms_pInstanceObject;
            }

            /*
                MPITypesFHelper implementation
            */
            
            MPICommsFHelper::MPICommsFHelper(): MPICommsHelper()
            {
            }

            
            MPICommsFHelper* MPICommsFHelper::ms_pInstanceObject = NULL;

            void MPICommsFHelper::fillMPICommsNames()
            {
                m_mpiCommsNames[MPICommsHelper::MC_MPI_COMM_WORLD] = "MPI_COMM_WORLD";
                m_mpiCommsNames[MPICommsHelper::MC_MPI_COMM_SELF]  = "MPI_COMM_SELF";
            }

            
            MPICommsFHelper& MPICommsFHelper::getInstance()
            {
                if(ms_pInstanceObject == NULL)
                {
                    ms_pInstanceObject = new MPICommsFHelper();
                }

                return *ms_pInstanceObject;
            }

            
            
            /*
                Implementation of MPIHelper class
            */
            const std::string MPIHelper::RANK_NOTE_NAME = "RANK_NOTE";
            const std::string MPIHelper::SIZE_NOTE_NAME = "SIZE_NOTE";
            const std::string MPIHelper::INIT_BLOCK_NOTE_NAME = "INIT_BLOCK_NOTE";

            MPIHelper::MPIHelper():
                m_subroutinbesDeclarationMap(),
                m_typesDeclarationMap(),
                m_communicatorsDeclarationsMap(),
                m_basicTypesDeclarationsMap()
            {
            }

            bool MPIHelper::validate()
            {
                // Check MPI functions
                for(int i = 0; i < MPIFunctionsHelper::MF_FUNCTIONS_COUNT; ++i)
                {
                    if(getMPIFunctionDeclaration((MPIFunctionsHelper::MPI_FUNCS)i) == NULL)
                    {
                        return false;
                    }
                }

                // Check MPI data types
                for(int i = 0; i < MPITypesHelper::MT_TYPES_COUNT; ++i)
                {
                    if(getMPITypeDeclaration((MPITypesHelper::MPI_TYPES)i) == NULL)
                    {
                        return false;
                    }
                }

                // Check MPI communicators
                for(int i = 0; i < MPICommsHelper::MC_MPI_COMM_COUNT; ++i)
                {
                    if(getMPICommDeclaration((MPICommsHelper::MPI_COMMS)i) == NULL)
                    {
                        return false;
                    }
                }

                // Check MPI basic types
                for(int i = 0; i < MPIBasicTypesHelper::MBT_MPI_BASIC_TYPES_COUNT; ++i)
                {
                    if(getMPIBasicTypeDeclaration((MPIBasicTypesHelper::MPI_BASIC_TYPES)i) == NULL)
                    {
                        return false;
                    }
                }

                return true;
            }

            SubroutineDeclaration* MPIHelper::getMPIFunctionDeclaration(MPIFunctionsHelper::MPI_FUNCS mpiFunctionKind)
            {
                OPS_ASSERT(mpiFunctionKind >= 0 && mpiFunctionKind < MPIFunctionsHelper::MF_FUNCTIONS_COUNT);

                SubroutineDeclaration* pResult = NULL;
                if(m_subroutinbesDeclarationMap.find(mpiFunctionKind) != m_subroutinbesDeclarationMap.end())
                {
                    pResult = m_subroutinbesDeclarationMap[mpiFunctionKind];
                }

                return pResult;
            }

            ReprisePtr<SubroutineCallExpression> MPIHelper::createMPIFunctionCallExpression(MPIFunctionsHelper::MPI_FUNCS mpiFunctionKind)
            {
                SubroutineReferenceExpression* mpiFuncReference = new SubroutineReferenceExpression(*getMPIFunctionDeclaration(mpiFunctionKind));
                ReprisePtr<SubroutineCallExpression> rpResult(new SubroutineCallExpression(mpiFuncReference));
                
                return rpResult;
            }

            TypeBase* MPIHelper::getMPITypeDeclaration(MPITypesHelper::MPI_TYPES mpiTypeKind)
            {
                OPS_ASSERT(mpiTypeKind >= 0 && mpiTypeKind < MPITypesHelper::MT_TYPES_COUNT);

                TypeBase* pResult = NULL;
                if(m_typesDeclarationMap.find(mpiTypeKind) != m_typesDeclarationMap.end())
                {
                    pResult = m_typesDeclarationMap[mpiTypeKind];
                }

                return pResult;
            }

            VariableDeclaration* MPIHelper::getMPICommDeclaration(MPICommsHelper::MPI_COMMS mpiCommKind)
            {
                OPS_ASSERT(mpiCommKind >= 0 && mpiCommKind < MPICommsHelper::MC_MPI_COMM_COUNT);

                VariableDeclaration* pResult = NULL;
                if(m_communicatorsDeclarationsMap.find(mpiCommKind) != m_communicatorsDeclarationsMap.end())
                {
                    pResult = m_communicatorsDeclarationsMap[mpiCommKind];
                }

                return pResult;
            }

            VariableDeclaration* MPIHelper::getMPIBasicTypeDeclaration(MPIBasicTypesHelper::MPI_BASIC_TYPES mpiBasicTypeKind)
            {
                OPS_ASSERT(mpiBasicTypeKind >= 0 && mpiBasicTypeKind < MPIBasicTypesHelper::MBT_MPI_BASIC_TYPES_COUNT);

                VariableDeclaration* pResult = NULL;
                if(m_basicTypesDeclarationsMap.find(mpiBasicTypeKind) != m_basicTypesDeclarationsMap.end())
                {
                    pResult = m_basicTypesDeclarationsMap[mpiBasicTypeKind];
                }
                
                return pResult;
            }

            MPIHelper::~MPIHelper()
            {
            }

            VariableDeclaration* MPIHelper::getRankDeclaration(SubroutineDeclaration& subroutine)
            {
                RankDeclarationFinderDeepWalker dw;
                subroutine.accept(dw);

                return dw.getRankDeclaration();
            }
            
            VariableDeclaration* MPIHelper::getSizeDeclaration(SubroutineDeclaration& subroutine)
            {
                SizeDeclarationFinderDeepWalker dw;
                subroutine.accept(dw);

                return dw.getSizeDeclaration();
            }

            BlockStatement* MPIHelper::getGlobalsInitBlock(SubroutineDeclaration& subroutine)
            {
                GlobalsInitBlockFinderDeepWalket dw;
                subroutine.accept(dw);

                return dw.getGlobalsInitBlock();
            }

            BlockStatement* MPIHelper::createGlobalsInitBlock(SubroutineDeclaration& subroutine)
            {
                ReprisePtr<BlockStatement> rpGlobalsInitBlock(new BlockStatement());
                subroutine.getBodyBlock().addFirst(rpGlobalsInitBlock.get());

                rpGlobalsInitBlock->setNote(MPIHelper::INIT_BLOCK_NOTE_NAME, Note::newBool(true));

                return rpGlobalsInitBlock.get();
            }

            /*
                Implementation of MPICHelper class
            */
            MPICHelper::MPICHelper(OPS::Reprise::TranslationUnit& translatinUnit): MPIHelper()
            {
                MPIProducerCFactory factory;

                // Find MPI functions
                for(int i = 0; i < MPIFunctionsHelper::MF_FUNCTIONS_COUNT; ++i)
                {
                    m_subroutinbesDeclarationMap[(MPIFunctionsHelper::MPI_FUNCS)i] = Shared::findSubroutineByName(&translatinUnit, factory.createMPIFunctionHelper()->getFunctionName((MPIFunctionsHelper::MPI_FUNCS)i));
                }

                // Find MPI types
                for(int i = 0; i < MPITypesHelper::MT_TYPES_COUNT; ++i)
                {
                    m_typesDeclarationMap[(MPITypesHelper::MPI_TYPES)i] = Shared::findDeclaredTypeByName(&translatinUnit, factory.createMPITypesHelper()->getMPITypeName((MPITypesHelper::MPI_TYPES)i));
                }

                IntegerHelper ih(BasicType::BT_INT32);
                
                // Find MPI communicators codes
                for(int i = 0; i < MPICommsHelper::MC_MPI_COMM_COUNT; ++i)
                {
                    m_communicatorsDeclarationsMap[(MPICommsHelper::MPI_COMMS)i] = translatinUnit.getGlobals().findVariable("__" + factory.createMPICommsHelper()->getMPICommName((MPICommsHelper::MPI_COMMS)i));
                }

                // Find MPI basic types codes
                for(int i = 0; i < MPIBasicTypesHelper::MBT_MPI_BASIC_TYPES_COUNT; ++i)
                {
                    m_basicTypesDeclarationsMap[(MPIBasicTypesHelper::MPI_BASIC_TYPES)i] = translatinUnit.getGlobals().findVariable("__" + factory.createMPIBasicTypesHelper()->getMPIBasicTypeName((MPIBasicTypesHelper::MPI_BASIC_TYPES)i));
                }
            }

            MPICHelper::~MPICHelper()
            {
            }

            ReprisePtr<ExpressionBase> MPIHelper::createMPICommCode( MPICommsHelper::MPI_COMMS mpiCommKind )
            {
                VariableDeclaration* pCommDeclaration = getMPICommDeclaration(mpiCommKind);

                OPS_ASSERT(pCommDeclaration != NULL);

                ReprisePtr<ExpressionBase> rpResult(new ReferenceExpression(*pCommDeclaration));

                return rpResult;
            }

            OPS::Reprise::ReprisePtr<OPS::Reprise::ExpressionBase> MPIHelper::createMPIBasicTypeCode( MPIBasicTypesHelper::MPI_BASIC_TYPES mpiBasicType)
            {
                VariableDeclaration* pTypeBaseDeclaration = getMPIBasicTypeDeclaration(mpiBasicType);

                OPS_ASSERT(pTypeBaseDeclaration != NULL);

                ReprisePtr<ExpressionBase> rpResult(new ReferenceExpression(*pTypeBaseDeclaration));

                return rpResult;
            }

            ReprisePtr<SubroutineCallExpression> MPICHelper::createMPIInitCallExpression(VariableDeclaration& argcDeclaration, VariableDeclaration& argvDeclaration)
            {
                ReprisePtr<SubroutineCallExpression> rpMPIInitCall = createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_INIT);
                rpMPIInitCall->addArgument(&R_AD() *(new ReferenceExpression(argcDeclaration)));
                rpMPIInitCall->addArgument(&R_AD() *(new ReferenceExpression(argvDeclaration)));

                return rpMPIInitCall;
            }

            ReprisePtr<SubroutineCallExpression> MPICHelper::createMPIFinalizeCallExpression()
            {
                ReprisePtr<SubroutineCallExpression> rpMPIFinalizeCall = createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_FINALIZE);

                return rpMPIFinalizeCall;
            }

            ReprisePtr<StatementBase> MPICHelper::createMPICommRankCallStatement(VariableDeclaration& rankDeclaration)
            {
                ReprisePtr<SubroutineCallExpression> rpMPICommRankCallExpr = createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_COMM_RANK);
                rpMPICommRankCallExpr->addArgument(createMPICommCode(MPICommsHelper::MC_MPI_COMM_WORLD).get());
                rpMPICommRankCallExpr->addArgument(&R_AD()(*(new ReferenceExpression(rankDeclaration))));

                ReprisePtr<StatementBase> rpResult(new ExpressionStatement(rpMPICommRankCallExpr.get()));
                return rpResult;
            }

            ReprisePtr<StatementBase> MPICHelper::createMPICommSizeCallStatement(VariableDeclaration& sizeDeclaration)
            {
                ReprisePtr<SubroutineCallExpression> rpMPICommSizeCallExpr = createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_COMM_SIZE);
                rpMPICommSizeCallExpr->addArgument(createMPICommCode(MPICommsHelper::MC_MPI_COMM_WORLD).get());
                rpMPICommSizeCallExpr->addArgument(&R_AD()(*(new ReferenceExpression(sizeDeclaration))));

                ReprisePtr<StatementBase> rpResult(new ExpressionStatement(rpMPICommSizeCallExpr.get()));
                return rpResult;
            }

            ReprisePtr<StatementBase> MPICHelper::createMPISendStatement(ExpressionBase& buf, ExpressionBase& count, ExpressionBase& datatype, ExpressionBase& dest, ExpressionBase& tag, ExpressionBase& comm)
            {
                ReprisePtr<SubroutineCallExpression> rpMPISendCallExpression = createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_SEND);

                rpMPISendCallExpression->addArgument(&buf);
                rpMPISendCallExpression->addArgument(&count);
                rpMPISendCallExpression->addArgument(&datatype);
                rpMPISendCallExpression->addArgument(&dest);
                rpMPISendCallExpression->addArgument(&tag);
                rpMPISendCallExpression->addArgument(&comm);

                ReprisePtr<StatementBase> result(new ExpressionStatement(rpMPISendCallExpression.get()));

                return result;
            }

            ReprisePtr<StatementBase> MPICHelper::createMPIRecvStatement(ExpressionBase& buf, ExpressionBase& count, ExpressionBase& datatype, ExpressionBase& source, ExpressionBase& tag, ExpressionBase& comm, ExpressionBase& status)
            {
                ReprisePtr<SubroutineCallExpression> rpMPIRecvCallExpression = createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_RECV);

                rpMPIRecvCallExpression->addArgument(&buf);
                rpMPIRecvCallExpression->addArgument(&count);
                rpMPIRecvCallExpression->addArgument(&datatype);
                rpMPIRecvCallExpression->addArgument(&source);
                rpMPIRecvCallExpression->addArgument(&tag);
                rpMPIRecvCallExpression->addArgument(&comm);
                rpMPIRecvCallExpression->addArgument(&status);

                ReprisePtr<StatementBase> result(new ExpressionStatement(rpMPIRecvCallExpression.get()));

                return result;
            }

            ReprisePtr<StatementBase> MPICHelper::createMPIWaitStatement( ExpressionBase& request, ExpressionBase& status )
            {
                ReprisePtr<SubroutineCallExpression> rpMPIWaitCallExpression = createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_WAIT);

                rpMPIWaitCallExpression->addArgument(&request);
                rpMPIWaitCallExpression->addArgument(&status);

                ReprisePtr<StatementBase> result(new ExpressionStatement(rpMPIWaitCallExpression.get()));

                return result;
            }

            ReprisePtr<StatementBase> MPICHelper::createMPIIRecvStatement( ExpressionBase& buf, ExpressionBase& count, ExpressionBase& datatype, ExpressionBase& source, ExpressionBase& tag, ExpressionBase& comm, ExpressionBase& request )
            {
                ReprisePtr<SubroutineCallExpression> rpMPIIRecvCallExpression = createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_IRECV);

                rpMPIIRecvCallExpression->addArgument(&buf);
                rpMPIIRecvCallExpression->addArgument(&count);
                rpMPIIRecvCallExpression->addArgument(&datatype);
                rpMPIIRecvCallExpression->addArgument(&source);
                rpMPIIRecvCallExpression->addArgument(&tag);
                rpMPIIRecvCallExpression->addArgument(&comm);
                rpMPIIRecvCallExpression->addArgument(&request);

                ReprisePtr<StatementBase> result(new ExpressionStatement(rpMPIIRecvCallExpression.get()));

                return result;
            }

            ReprisePtr<StatementBase> MPICHelper::createMPIISendStatement( ExpressionBase& buf, ExpressionBase& count, ExpressionBase& datatype, ExpressionBase& dest, ExpressionBase& tag, ExpressionBase& comm, ExpressionBase& request )
            {
                ReprisePtr<SubroutineCallExpression> rpMPIISendCallExpression = createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_ISEND);

                rpMPIISendCallExpression->addArgument(&buf);
                rpMPIISendCallExpression->addArgument(&count);
                rpMPIISendCallExpression->addArgument(&datatype);
                rpMPIISendCallExpression->addArgument(&dest);
                rpMPIISendCallExpression->addArgument(&tag);
                rpMPIISendCallExpression->addArgument(&comm);
                rpMPIISendCallExpression->addArgument(&request);

                ReprisePtr<StatementBase> result(new ExpressionStatement(rpMPIISendCallExpression.get()));

                return result;
            }

            ReprisePtr<StatementBase> MPICHelper::createMPIBcastStatement( ExpressionBase& buf, ExpressionBase& count, ExpressionBase& datatype, ExpressionBase& root, ExpressionBase& comm )
            {
                ReprisePtr<SubroutineCallExpression> rpMPIBcastCallExpression = createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_BCAST);

                rpMPIBcastCallExpression->addArgument(&buf);
                rpMPIBcastCallExpression->addArgument(&count);
                rpMPIBcastCallExpression->addArgument(&datatype);
                rpMPIBcastCallExpression->addArgument(&root);
                rpMPIBcastCallExpression->addArgument(&comm);

                ReprisePtr<StatementBase> result(new ExpressionStatement(rpMPIBcastCallExpression.get()));

                return result;
            }
        }
    }
}
