#include "Transforms/Loops/LoopSplitting/LoopSplitting.h"
#include "Reprise/ServiceFunctions.h"

namespace OPS
{
    namespace Transforms
    {
        namespace Loops
        {

            using namespace OPS::Reprise;

            std::pair<Reprise::ForStatement*, Reprise::ForStatement*>
                makeLoopSplitting(Reprise::ForStatement* forStatement, Reprise::ExpressionBase* minSplittingExpression,
                                  Reprise::ExpressionBase* maxSplittingExpression)
            {

                if(!Editing::forIsBasic(*forStatement))
                    throw OPS::Exception("Invalid loop type");

                Reprise::ReprisePtr<Reprise::ExpressionBase> minSplittingExpressionPtr(minSplittingExpression);
                Reprise::ReprisePtr<Reprise::ExpressionBase> maxSplittingExpressionPtr(maxSplittingExpression);

                BlockStatement& parentBlock = forStatement->getParentBlock();


                //init new loop
                Reprise::ForStatement* newForStatement = forStatement->clone();
                parentBlock.addAfter(parentBlock.convertToIterator(forStatement), newForStatement);

                //change first loop
                BasicCallExpression& finalBasicExpression = forStatement->getFinalExpression().cast_to<BasicCallExpression>();
                ExpressionBase& finalExpression = finalBasicExpression.getArgument(1);
                finalBasicExpression.replaceArgument(finalExpression, minSplittingExpressionPtr);

                //change second loop
                BasicCallExpression& initBasicExpression = newForStatement->getInitExpression().cast_to<BasicCallExpression>();
                ExpressionBase& initExpression = initBasicExpression.getArgument(1);
                initBasicExpression.replaceArgument(initExpression, maxSplittingExpressionPtr);


                return std::make_pair(forStatement, newForStatement);

            }

            std::pair<Reprise::ForStatement*, Reprise::ForStatement*>
                makeLoopSplittingSafe(Reprise::ForStatement* forStatement, Reprise::ExpressionBase* splittingExpression)
            {

                if(!Editing::forIsBasic(*forStatement))
                    throw OPS::Exception("Invalid loop type");

                ExpressionBase& basicForFinalExpression = Editing::getBasicForFinalExpression(*forStatement);

                //(N>100)?100:N
                ExpressionBase* minConditionArg = new BasicCallExpression(BasicCallExpression::BCK_GREATER, basicForFinalExpression.clone(), splittingExpression->clone());
                BasicCallExpression* minSplittingExpression = new BasicCallExpression(BasicCallExpression::BCK_CONDITIONAL);
                minSplittingExpression->addArgument(minConditionArg);
                minSplittingExpression->addArgument(splittingExpression->clone());
                minSplittingExpression->addArgument(basicForFinalExpression.clone());

                //(N<100)?100:N
                ExpressionBase* maxConditionArg = new BasicCallExpression(BasicCallExpression::BCK_LESS, basicForFinalExpression.clone(), splittingExpression->clone());
                BasicCallExpression* maxSplittingExpression = new BasicCallExpression(BasicCallExpression::BCK_CONDITIONAL);
                maxSplittingExpression->addArgument(maxConditionArg);
                maxSplittingExpression->addArgument(splittingExpression->clone());
                maxSplittingExpression->addArgument(basicForFinalExpression.clone());

                return makeLoopSplitting(forStatement,
                                         minSplittingExpression,
                                         maxSplittingExpression);

            }


        } // Scalar
    } // OPS
} // Transforms
