#include "Transforms/Scalar/LocalizationScalar/LocalizationScalar.h"
#include "Reprise/Service/DeepWalker.h"

namespace OPS
{
    namespace Transforms
    {
        namespace Scalar
        {

            using namespace OPS::Reprise;


            class LocalizationFinder: public OPS::Reprise::Service::DeepWalker
            {
            private:
                VariableDeclaration* m_variableDeclaration;
                VariableDeclaration* m_oldVariableDeclaration;
            public:

                LocalizationFinder(VariableDeclaration* variableDeclaration,
                                   VariableDeclaration* oldVariableDeclaration)

                {
                    m_variableDeclaration = variableDeclaration;
                    m_oldVariableDeclaration = oldVariableDeclaration;
                }


                void visit(ReferenceExpression& referenceExpression)
                {
                    VariableDeclaration& referenceIn = referenceExpression.getReference();
                    if (&referenceIn == m_oldVariableDeclaration)
                    {
                        referenceExpression.setReference(m_variableDeclaration);
                    }
                    DeepWalker::visit(referenceExpression);
                }
            };

            LocalizationScalar::LocalizationScalar(Reprise::ProgramFragment& program,
                                                   Analysis::OccurrencesByDeclarations& in,
                                                   Analysis::OccurrencesByDeclarations& out):
													m_inOccurrencesByDeclarations(in),
													m_outOccurrencesByDeclarations(out),
													m_fragment(program)
            {
            }


            void LocalizationScalar::makeTransform()
            {

                if ((m_inOccurrencesByDeclarations.size() == 0) && (m_outOccurrencesByDeclarations.size() == 0)) return;

                //new BlockStatement
                BlockStatement& fragmentBlock = m_fragment.getStatementsBlock();
                BlockStatement* blockStatement = new BlockStatement();

                fragmentBlock.addAfter(m_fragment.getLastIterator(), blockStatement);

                BlockStatement::Iterator endIter = m_fragment.getAfterLastIterator();
                BlockStatement::Iterator itStmt = m_fragment.getFirstIterator();
                for(; itStmt != endIter;)
                {
                    ReprisePtr<StatementBase> stmt(&*itStmt);
                    BlockStatement::Iterator tmpIt = itStmt++;
                    fragmentBlock.erase(tmpIt);
                    blockStatement->addLast(stmt.get());
                }

                m_fragment.setFromBlock(*blockStatement);


                Analysis::OccurrencesByDeclarations::const_iterator itOc;
                for ( itOc = m_inOccurrencesByDeclarations.begin(); itOc != m_inOccurrencesByDeclarations.end(); ++itOc)                
                {
                    if (!Reprise::Editing::desugarType(itOc->first->getType()).is_a<BasicType>())
                        continue;
                    Analysis::OccurrencesByDeclarations::const_iterator itOo = m_outOccurrencesByDeclarations.find(itOc->first);
                    if (itOo == m_outOccurrencesByDeclarations.end())
                        continue;

                    VariableDeclaration* vd = const_cast<VariableDeclaration *>(itOc->first);

                    //new local
                    VariableDeclaration& new_vd = Editing::createNewVariable(vd->getType(), *blockStatement, "local_"+vd->getName());

                    //local = a
                    ReferenceExpression* leftNewVdReferenceExpression = new ReferenceExpression(new_vd);
                    ReferenceExpression* rightVdReferenceExpression = new ReferenceExpression(*vd);
                    BasicCallExpression* basicCallExpressionNewVdVd = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN);
                    basicCallExpressionNewVdVd->addArgument(leftNewVdReferenceExpression);
                    basicCallExpressionNewVdVd->addArgument(rightVdReferenceExpression);
                    ExpressionStatement* expressionStatementNewVdVd = new ExpressionStatement(basicCallExpressionNewVdVd);
                    blockStatement->addBefore(m_fragment.getFirstIterator(), expressionStatementNewVdVd);

                    //replace references
                    //procesing with in

                    LocalizationFinder localizationFinder(&new_vd, vd);
                    m_fragment.accept(localizationFinder);


                    //new out
                    BlockStatement& definedBlock = vd->getDefinedBlock();
                    VariableDeclaration& new_vd_out = Editing::createNewVariable(vd->getType(), definedBlock, "out_"+vd->getName());



                    //out = local
                    ReferenceExpression* leftOutReferenceExpressionNewVdOut = new ReferenceExpression(new_vd_out);
                    ReferenceExpression* rightVdReferenceExpressionNewVd = new ReferenceExpression(new_vd);
                    BasicCallExpression* basicCallExpressionNewVdOutNewVd = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN);
                    basicCallExpressionNewVdOutNewVd->addArgument(leftOutReferenceExpressionNewVdOut);
                    basicCallExpressionNewVdOutNewVd->addArgument(rightVdReferenceExpressionNewVd);
                    ExpressionStatement* expressionStatementNewVdOutNewVd = new ExpressionStatement(basicCallExpressionNewVdOutNewVd);
                    blockStatement->addAfter(m_fragment.getLastIterator(), expressionStatementNewVdOutNewVd);

                    //remove variable a from out list and add variable out to out list

                    m_outOccurrencesByDeclarations.erase(itOc->first);
                    Analysis::TopLevelOccurrenceList occurrenceList;
                    occurrenceList.push_back(leftOutReferenceExpressionNewVdOut);
                    m_outOccurrencesByDeclarations.insert(std::make_pair(&new_vd_out, occurrenceList));

                    //a = out
                    ReferenceExpression* leftOutReferenceExpressionVd = new ReferenceExpression(*vd);
                    ReferenceExpression* rightVdReferenceExpressionNewVdOut = new ReferenceExpression(new_vd_out);
                    BasicCallExpression* basicCallExpressionVdNewVdOut = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN);
                    basicCallExpressionVdNewVdOut->addArgument(leftOutReferenceExpressionVd);
                    basicCallExpressionVdNewVdOut->addArgument(rightVdReferenceExpressionNewVdOut);
                    ExpressionStatement* expressionStatementVdNewVdOut = new ExpressionStatement(basicCallExpressionVdNewVdOut);
                    fragmentBlock.addAfter(fragmentBlock.convertToIterator(blockStatement), expressionStatementVdNewVdOut);

                }

              m_fragment.setFromBlock(*blockStatement);

            }

            Reprise::ProgramFragment& LocalizationScalar::getProgram()
            {
                return m_fragment;
            }

            Analysis::OccurrencesByDeclarations& LocalizationScalar::getIn()
            {
                return m_inOccurrencesByDeclarations;
            }

            Analysis::OccurrencesByDeclarations& LocalizationScalar::getOut()
            {
                return m_outOccurrencesByDeclarations;
            }

        } // Scalar
    } // OPS
} // Transforms
