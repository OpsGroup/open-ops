#ifndef _STATUS_
#define _STATUS_

namespace DepGraph
{
class Status
{
public:
	typedef unsigned int status_t;

	explicit Status(status_t s = 0):status(s){}
//	Status(int s):status(s){}
	Status(const Status& s):status(s.GetStatusData()){}
	Status& operator=(const Status& s){status=s.GetStatusData();return *this;}
	void	 SetStatus(status_t s) { status |= s; }
	void	 SaveStatus(status_t s){ status = s; }
	bool	 isStatus(status_t s) const { return (status & s) != 0; }
	void	 ClearStatus(status_t s) { status &= ~s; }
	status_t GetStatusData() const { return status; }
	void	 UnsetStatus(status_t s) { status &= ~s; }//the same as "ClearStatus"
private:
	status_t status;
};
}

#endif
