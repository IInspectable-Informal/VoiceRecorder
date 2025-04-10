#include "pch.h"
#include "helpers.h"

//STL
#include <cmath>

using ns winrt;
using ns Windows::Foundation;
using ns Windows::Media::MediaProperties;
using ns Windows::Storage;
using ns Windows::UI::Xaml;
using ns Windows::UI::Xaml::Data;

namespace winrt
{
    hstring GetCurrentDateTime(hstring const& format)
    {
        Windows::Globalization::DateTimeFormatting::DateTimeFormatter formatter(format);
        hstring datetimeStr = formatter.Format(winrt::clock::now());
        formatter = nullptr;
        return datetimeStr;
    }

    double DoubleDivision(double const& dividend, double const& divisor)
    {
        if (divisor == 0)
        { throw hresult_invalid_argument(L"Divisor must not be 0!"); }
        return std::floor(dividend / divisor);
    }

    hstring DoubleToFormattedString(double const& num, uint32_t const& minSize, wchar_t const& filler)
    {
        hstring numstr = to_hstring(num);
        int64_t neddedCount = minSize - to_hstring(std::floor(std::abs(num))).size(); 
        if (neddedCount <= 0)
        { return numstr; }
        for (uint32_t i = 0; i < neddedCount; i++)
        { numstr = filler + numstr; }
        return numstr;
    }

    hstring ToHex(long const& hresult)
    {
        if (hresult == 0) { return L"0"; }
        hstring hexHstr = L"";
        unsigned int n = static_cast<unsigned int>(hresult);
        while (n > 0)
        { hexHstr = hexChars[n % 16] + hexHstr; n /= 16; }
        return hexHstr;
    }

    MediaEncodingProfile GetEncodingProfile(int const& index, AudioEncodingQuality const& quality)
    {
        switch (index)
        {
            case 0: return MediaEncodingProfile::CreateAlac(quality);
            case 1: return MediaEncodingProfile::CreateFlac(quality);
            case 2: return MediaEncodingProfile::CreateM4a(quality);
            case 3: return MediaEncodingProfile::CreateMp3(quality);
            case 4: return MediaEncodingProfile::CreateWav(quality);
            case 5: return MediaEncodingProfile::CreateWma(quality);
            default: throw hresult_invalid_argument(L"Invalid index!");
        }
    }

    bool CompareFilesByDate(const StorageFile& file1, const StorageFile& file2)
    { return file1.DateCreated() > file2.DateCreated(); }

    void SetBinder(FrameworkElement const& target,
                   DependencyProperty const& targetProp,
                   IInspectable const& source,
                   BindingMode const& mode,
                   IValueConverter const& converter,
                   IInspectable const& targetNullValue,
                   IInspectable const& fallbackValue,
                   IInspectable const& param) {
        Binding binder;
        binder.Source(source);
        binder.Mode(mode);
        if (converter)
        {
            binder.Converter(converter);
            binder.ConverterParameter(param);
        }
        if (targetNullValue)
        { binder.TargetNullValue(targetNullValue); }
        if (fallbackValue)
        { binder.FallbackValue(fallbackValue); }
        target.SetBinding(targetProp, binder);
    }
}
