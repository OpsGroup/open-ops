/// Transforms/Scalar/Silly/SillyTranslationUnit.h
///   Test transformation applied to the translation unit.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created: 20.11.2013

#ifndef SILLYTRANSLATIONUNIT_H__
#define SILLYTRANSLATIONUNIT_H__

namespace OPS
{
	namespace Reprise
	{
		class TranslationUnit;
	}

	namespace Transforms
	{
		namespace Scalar
		{
			void applySillyToTranslationUnit(
				OPS::Reprise::TranslationUnit& rTranslationUnit);

		}    // namespace Scalar
	}    // namespace Transforms
}    // namespace OPS

#endif    // SILLYTRANSLATIONUNIT_H__

// End of File
