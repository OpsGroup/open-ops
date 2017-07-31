#ifndef _CORE_H_INCLUDED_
#define _CORE_H_INCLUDED_

namespace OPS
{
	namespace Core
	{
		class Core
		{
		public:

			Core();

			~Core();

		private:

			Core(const Core& );
			Core& operator =(const Core& );

			void initialize();
			void terminate();
		};
	}
}

#endif // _CORE_H_INCLUDED_
