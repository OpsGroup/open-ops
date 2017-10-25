#include "Reprise/Reprise.h"
#include <list>

struct ExtendedQuast;
struct ParamPoint;

namespace OPS
{
namespace LatticeGraph
{

class Correspondence : public OPS::NonCopyableMix
{
typedef std::vector<ExtendedQuast*> FunctionVector;

public:
    Correspondence() {}
    explicit Correspondence(FunctionVector directFunctions);
    Correspondence(FunctionVector directFunctions, FunctionVector inverseFunctions);
    ~Correspondence();
    
    Correspondence* clone();//копирование

    Correspondence* buildInverseCorrespondence();//обращение

    //Вычисление образа 1 точки arg. Получающийся образ будет состоять из объединения списка точек и ККАМ
    //Параметры точки и параметры кваста считаются различными!!!!!!!!!!!!!!!!!!!!!!!!
    void evaluateDirectImage(ParamPoint arg, std::list<ParamPoint>& resultPoints, ExtendedQuast& resultPolyhedra);

    //Вычисление прообраза 1 точки arg. Получающийся прообраз будет состоять из объединения списка точек и ККАМ
    //Параметры точки и параметры кваста считаются различными!!!!!!!!!!!!!!!!!!!!!!!!
    void evaluateInverseImage(ParamPoint arg, std::list<ParamPoint>& resultPoints, ExtendedQuast& resultPolyhedra);

private:
    
    //ККА-функции определяющие соответствие
    FunctionVector m_directFunctions;
    FunctionVector m_inverseFunctions;


};

}//end of namespace
}//end of namespace
