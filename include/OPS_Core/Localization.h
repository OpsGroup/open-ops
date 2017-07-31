#ifndef OPS_COMMON_CORE_LOCALIZATION_H_
#define OPS_COMMON_CORE_LOCALIZATION_H_

#if defined (_TL)
#error _TL already defined
#endif

#if !defined (OPS_LOCALE)
#define OPS_LOCALE 0
#endif

#if OPS_LOCALE == 0
#define OPS_LOCALE_ENGLISH 1
#define OPS_LOCALE_RUSSIAN 0
#elif OPS_LOCALE == 1
#define OPS_LOCALE_ENGLISH 0
#define OPS_LOCALE_RUSSIAN 1
#else
#error Unexpected OPS_LOCALE or not defined.
#endif

#if OPS_LOCALE_ENGLISH
#define _TL(english, russian) english
#elif OPS_LOCALE_RUSSIAN
#define _TL(english, russian) russian
#else
#error Unexpected OPS_LOCALE, could not define _TL.
#endif


#endif	// OPS_COMMON_CORE_LOCALIZATION_H_
