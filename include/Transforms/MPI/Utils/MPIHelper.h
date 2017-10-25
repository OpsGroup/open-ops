#ifndef MPI_HELPER_H
#define MPI_HELPER_H

#include <map>

#include "Reprise/Reprise.h"

namespace OPS
{
    namespace Transforms
    {
        namespace MPIProducer
        {
            /**
                Class that represents MPI_FUNCTION enum and encapsulates mapping between this enum and MPI function names.
            */
            class MPIFunctionsHelper
            {
            public:
                enum MPI_FUNCS
                {
                    MF_MPI_INIT = 0,
                    MF_MPI_FINALIZE,
                    MF_MPI_COMM_RANK,
                    MF_MPI_COMM_SIZE,
                    MF_MPI_TYPE_VECTOR,
                    MF_MPI_TYPE_HVECTOR,
                    MF_MPI_TYPE_EXTENT,
                    MF_MPI_TYPE_COMMIT,
                    MF_MPI_TYPE_FREE,
                    MF_MPI_SEND,
                    MF_MPI_ISEND,
                    MF_MPI_WAIT,
                    MF_MPI_RECV,
                    MF_MPI_IRECV,
                    MF_MPI_ALLGATHER,
                    MF_MPI_BCAST,

                    MF_FUNCTIONS_COUNT
                };

                typedef std::map<MPI_FUNCS, std::string> MPIFunctionsNames;

            protected:
                MPIFunctionsHelper();

                virtual void fillMPIFunctionsNames() = 0;

            public:
                std::string getFunctionName(MPI_FUNCS mpiFunctionKind) const;

                virtual ~MPIFunctionsHelper();

            protected:
                MPIFunctionsNames m_mpiFunctionsNames;
            };

            /**
                Class that represents MPI_FUNCTION enum and encapsulates mapping between this enum and C MPI function names.
                Realize singleton pattern
            */
            class MPIFunctionsCHelper: public MPIFunctionsHelper
            {
            protected:
                MPIFunctionsCHelper();

                virtual void fillMPIFunctionsNames();

            public:
                static MPIFunctionsCHelper& getInstance();

            protected:
                static MPIFunctionsCHelper* ms_pInstanceObject;
            };

            /**
                Class that repesents MPI_FUNCTION enum and incapsulates mapping between this enum and Fortran MPI function names.
                Realise singleton pattern
            */
            class MPIFunctionsFHelper: public MPIFunctionsHelper
            {
            protected:
                MPIFunctionsFHelper();

                virtual void fillMPIFunctionsNames();

            public:
                static MPIFunctionsFHelper& getInstance();

            protected:
                static MPIFunctionsFHelper* ms_pInstanceObject;
            };


            /**
                Class that repesents MPI_BASIC_TYPES enum and incapsulates mapping between this enum and MPI basic types names.
            */
            class MPIBasicTypesHelper
            {
            public:
                enum MPI_BASIC_TYPES
                {
                    MBT_MPI_INTEGER1 = 0,
                    MBT_MPI_INTEGER2,
                    MBT_MPI_INTEGER4,
                    MBT_MPI_INTEGER8,

                    MBT_MPI_REAL4,
                    MBT_MPI_REAL8,

                    MBT_MPI_CHAR,

                    MBT_MPI_BASIC_TYPES_COUNT
                };

                typedef std::map<MPI_BASIC_TYPES, std::string> MPIBasicTypesNames;
                typedef std::map<OPS::Reprise::BasicType::BasicTypes, MPI_BASIC_TYPES> Reprise2MPITipesMap;

            protected:
                MPIBasicTypesHelper();

                virtual void fillMPIBasicTypesNames() = 0;
                virtual void fillReprise2MPITipesMap() = 0;

            public:
                std::string getMPIBasicTypeName(MPI_BASIC_TYPES mpiBasicTypeKind) const;
                MPI_BASIC_TYPES getMPIBasicTypeByRepriseBasicType(OPS::Reprise::BasicType::BasicTypes repriseBasicTypeKind) const;

                virtual ~MPIBasicTypesHelper();

            protected:
                MPIBasicTypesNames  m_mpiBasicTypesNames;
                Reprise2MPITipesMap m_reprise2MPITipesMap;
            };

            /**
                Class that repesents MPI_BASIC_TYPES enum and incapsulates mapping between this enum and C MPI basic types names.
                Realise singleton pattern
            */
            class MPIBasicTypesCHelper: public MPIBasicTypesHelper
            {
            protected:
                MPIBasicTypesCHelper();

                virtual void fillMPIBasicTypesNames();
                virtual void fillReprise2MPITipesMap();

            public:
                static MPIBasicTypesCHelper& getInstance();

            protected:
                static MPIBasicTypesCHelper* ms_pInstanceObject;
            };

            /**
                Class that repesents MPI_BASIC_TYPES enum and incapsulates mapping between this enum and Fortran MPI basic types names.
                Realise singleton pattern
            */
            class MPIBasicTypesFHelper: public MPIBasicTypesHelper
            {
            protected:
                MPIBasicTypesFHelper();

                virtual void fillMPIBasicTypesNames();
                virtual void fillReprise2MPITipesMap();

            public:
                static MPIBasicTypesFHelper& getInstance();

            protected:
                static MPIBasicTypesFHelper* ms_pInstanceObject;
            };

            /**
                Class that repesents MPI_TYPES enum and incapsulates mapping between this enum and MPI types names.
            */
            class MPITypesHelper
            {
            public:
                enum MPI_TYPES
                {
                    MT_MPI_DATATYPE = 0,
                    MT_MPI_COMM,
                    MT_MPI_STATUS,
                    MT_MPI_REQUEST,

                    MT_TYPES_COUNT
                };

                typedef std::map<MPI_TYPES, std::string> MPITypesNames;

            protected:
                MPITypesHelper();

                virtual void fillMPITypesNames() = 0;

            public:
                std::string getMPITypeName(MPI_TYPES mpiTypeKind) const;
                
                virtual ~MPITypesHelper();

            protected:
                MPITypesNames m_mpiTypesNames;
            };

            /**
                Class that repesents MPI_TYPES enum and incapsulates mapping between this enum and C MPI types names.
                Realise singleton pattern
            */
            class MPITypesCHelper: public MPITypesHelper
            {
            protected:
                MPITypesCHelper();

                virtual void fillMPITypesNames();

            public:
                static MPITypesCHelper& getInstance();

            protected:
                static MPITypesCHelper* ms_pInstanceObject;
            };

            /**
                Class that repesents MPI_TYPES enum and incapsulates mapping between this enum and Fortran MPI types names.
                Realise singleton pattern
            */
            class MPITypesFHelper: public MPITypesHelper
            {
            protected:
                MPITypesFHelper();

                virtual void fillMPITypesNames();

            public:
                static MPITypesFHelper& getInstance();

            protected:
                static MPITypesFHelper* ms_pInstanceObject;
            };

            /**
                Class that repesents MPI_COMMS enum and incapsulates mapping between this enum and MPI communicators names.
            */
            class MPICommsHelper
            {
            public:
                enum MPI_COMMS
                {
                    MC_MPI_COMM_WORLD = 0,
                    MC_MPI_COMM_SELF,

                    MC_MPI_COMM_COUNT
                };

                typedef std::map<MPI_COMMS, std::string> MPICommsNames;

            protected:
                MPICommsHelper();

                virtual void fillMPICommsNames() = 0;

            public:
                virtual ~MPICommsHelper();

                std::string getMPICommName(MPI_COMMS mpiCommKind) const;

            protected:
                MPICommsNames m_mpiCommsNames;
            };

            /**
                Class that repesents MPI_COMMS enum and incapsulates mapping between this enum and C MPI communicators names.
                Realise singleton pattern
            */
            class MPICommsCHelper: public MPICommsHelper
            {
            protected:
                MPICommsCHelper();

                virtual void fillMPICommsNames();

            public:
                static MPICommsCHelper& getInstance();

            protected:
                static MPICommsCHelper* ms_pInstanceObject;
            };

            /**
                Class that repesents MPI_COMMS enum and incapsulates mapping between this enum and Fortran MPI communicators names.
                Realise singleton pattern
            */
            class MPICommsFHelper: public MPICommsHelper
            {
            protected:
                MPICommsFHelper();

                virtual void fillMPICommsNames();

            public:
                static MPICommsFHelper& getInstance();

            protected:
                static MPICommsFHelper* ms_pInstanceObject;
            };


            
            /**
                Provides access to MPI functions, constants etc.
            */
            class MPIHelper
            {
            public:
                static const std::string RANK_NOTE_NAME;
                static const std::string SIZE_NOTE_NAME;
                static const std::string INIT_BLOCK_NOTE_NAME;

            public:
                class MPIHelperException: public OPS::Exception
                {
                public:
                    MPIHelperException(std::string message): OPS::Exception(message) {};
                };

                typedef std::map<MPIFunctionsHelper::MPI_FUNCS, OPS::Reprise::SubroutineDeclaration*> MPISubroutineDeclarationsMap;
                typedef std::map<MPITypesHelper::MPI_TYPES, OPS::Reprise::TypeBase*> MPITypesMap;
                typedef std::map<MPICommsHelper::MPI_COMMS, OPS::Reprise::VariableDeclaration*> MPICommunicatorsDeclarationsMap;
                typedef std::map<MPIBasicTypesHelper::MPI_BASIC_TYPES, OPS::Reprise::VariableDeclaration*> MPIBasicTypesDeclarationsMap;

            public:
                MPIHelper();
				virtual ~MPIHelper();
                
                virtual bool validate();

                virtual OPS::Reprise::SubroutineDeclaration* getMPIFunctionDeclaration(MPIFunctionsHelper::MPI_FUNCS mpiFunctionKind);
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::SubroutineCallExpression> createMPIFunctionCallExpression(MPIFunctionsHelper::MPI_FUNCS mpiFunctionKind);
                virtual OPS::Reprise::TypeBase* getMPITypeDeclaration(MPITypesHelper::MPI_TYPES mpiTypeKind);
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::ExpressionBase> createMPICommCode(MPICommsHelper::MPI_COMMS mpiCommKind);
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::ExpressionBase> createMPIBasicTypeCode(MPIBasicTypesHelper::MPI_BASIC_TYPES mpiBasicType);
                virtual OPS::Reprise::VariableDeclaration* getMPICommDeclaration(MPICommsHelper::MPI_COMMS mpiCommKind);
                virtual OPS::Reprise::VariableDeclaration* getMPIBasicTypeDeclaration(MPIBasicTypesHelper::MPI_BASIC_TYPES mpiBasicType);

                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::SubroutineCallExpression> createMPIInitCallExpression(OPS::Reprise::VariableDeclaration& argcDeclaration, OPS::Reprise::VariableDeclaration& argvDeclaration) = 0;
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::SubroutineCallExpression> createMPIFinalizeCallExpression() = 0;
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPICommRankCallStatement(OPS::Reprise::VariableDeclaration& rankDeclaration) = 0;
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPICommSizeCallStatement(OPS::Reprise::VariableDeclaration& rankDeclaration) = 0;
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPISendStatement(OPS::Reprise::ExpressionBase& buf, OPS::Reprise::ExpressionBase& count, OPS::Reprise::ExpressionBase& datatype, OPS::Reprise::ExpressionBase& dest, OPS::Reprise::ExpressionBase& tag, OPS::Reprise::ExpressionBase& comm) = 0;
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPIRecvStatement(OPS::Reprise::ExpressionBase& buf, OPS::Reprise::ExpressionBase& count, OPS::Reprise::ExpressionBase& datatype, OPS::Reprise::ExpressionBase& source, OPS::Reprise::ExpressionBase& tag, OPS::Reprise::ExpressionBase& comm, OPS::Reprise::ExpressionBase& status) = 0;
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPIISendStatement(OPS::Reprise::ExpressionBase& buf, OPS::Reprise::ExpressionBase& count, OPS::Reprise::ExpressionBase& datatype, OPS::Reprise::ExpressionBase& dest, OPS::Reprise::ExpressionBase& tag, OPS::Reprise::ExpressionBase& comm, OPS::Reprise::ExpressionBase& request) = 0;
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPIIRecvStatement(OPS::Reprise::ExpressionBase& buf, OPS::Reprise::ExpressionBase& count, OPS::Reprise::ExpressionBase& datatype, OPS::Reprise::ExpressionBase& source, OPS::Reprise::ExpressionBase& tag, OPS::Reprise::ExpressionBase& comm, OPS::Reprise::ExpressionBase& request) = 0;
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPIWaitStatement(OPS::Reprise::ExpressionBase& request, OPS::Reprise::ExpressionBase& status) = 0;
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPIBcastStatement(OPS::Reprise::ExpressionBase& buf, OPS::Reprise::ExpressionBase& count, OPS::Reprise::ExpressionBase& datatype, OPS::Reprise::ExpressionBase& root, OPS::Reprise::ExpressionBase& comm) = 0;

                static OPS::Reprise::VariableDeclaration* getRankDeclaration(OPS::Reprise::SubroutineDeclaration& subroutine);
                static OPS::Reprise::VariableDeclaration* getSizeDeclaration(OPS::Reprise::SubroutineDeclaration& subroutine);
                static OPS::Reprise::BlockStatement* getGlobalsInitBlock(OPS::Reprise::SubroutineDeclaration& subroutine);
                static OPS::Reprise::BlockStatement* createGlobalsInitBlock(OPS::Reprise::SubroutineDeclaration& subroutine);

            protected:
                MPISubroutineDeclarationsMap    m_subroutinbesDeclarationMap;
                MPITypesMap                     m_typesDeclarationMap;
                MPICommunicatorsDeclarationsMap m_communicatorsDeclarationsMap;
                MPIBasicTypesDeclarationsMap    m_basicTypesDeclarationsMap;
            };

            class MPICHelper: public MPIHelper
            {
            public:
                MPICHelper(OPS::Reprise::TranslationUnit& pTranslatinUnit);

                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::SubroutineCallExpression> createMPIInitCallExpression(OPS::Reprise::VariableDeclaration& argcDeclaration, OPS::Reprise::VariableDeclaration& argvDeclaration);
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::SubroutineCallExpression> createMPIFinalizeCallExpression();
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPICommRankCallStatement(OPS::Reprise::VariableDeclaration& rankDeclaration);
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPICommSizeCallStatement(OPS::Reprise::VariableDeclaration& rankDeclaration);
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPISendStatement(OPS::Reprise::ExpressionBase& buf, OPS::Reprise::ExpressionBase& count, OPS::Reprise::ExpressionBase& datatype, OPS::Reprise::ExpressionBase& dest, OPS::Reprise::ExpressionBase& tag, OPS::Reprise::ExpressionBase& comm);
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPIRecvStatement(OPS::Reprise::ExpressionBase& buf, OPS::Reprise::ExpressionBase& count, OPS::Reprise::ExpressionBase& datatype, OPS::Reprise::ExpressionBase& source, OPS::Reprise::ExpressionBase& tag, OPS::Reprise::ExpressionBase& comm, OPS::Reprise::ExpressionBase& status);
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPIIRecvStatement(OPS::Reprise::ExpressionBase& buf, OPS::Reprise::ExpressionBase& count, OPS::Reprise::ExpressionBase& datatype, OPS::Reprise::ExpressionBase& source, OPS::Reprise::ExpressionBase& tag, OPS::Reprise::ExpressionBase& comm, OPS::Reprise::ExpressionBase& request);
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPIISendStatement(OPS::Reprise::ExpressionBase& buf, OPS::Reprise::ExpressionBase& count, OPS::Reprise::ExpressionBase& datatype, OPS::Reprise::ExpressionBase& dest, OPS::Reprise::ExpressionBase& tag, OPS::Reprise::ExpressionBase& comm, OPS::Reprise::ExpressionBase& request);
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPIWaitStatement(OPS::Reprise::ExpressionBase& request, OPS::Reprise::ExpressionBase& status);
                virtual OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> createMPIBcastStatement(OPS::Reprise::ExpressionBase& buf, OPS::Reprise::ExpressionBase& count, OPS::Reprise::ExpressionBase& datatype, OPS::Reprise::ExpressionBase& root, OPS::Reprise::ExpressionBase& comm);

                ~MPICHelper();
            };
        }
    }
}

#endif // MPI_HELPER_H



