#include <exception>
#include <winnt.h>
#include <stdio.h>
#include <winerror.h>
namespace DX {
	// Helper class for COM exceptions.
	class com_exception : public std::exception {
	public:
		com_exception(HRESULT hr) : result{ hr }{}
		// pointer to a constant char. 
		// will throw a compiler error for this class function to change a data member of the class.
		// Expression is declared to not throw any exceptions. 
		const char* what() const noexcept override {
			static char s_str[64]{};
			sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
			return s_str;
		}
	private:
		HRESULT result;
	};
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			auto err = com_exception(hr).what();
			throw com_exception(hr);
		}
	}
}