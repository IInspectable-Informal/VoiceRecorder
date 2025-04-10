#pragma once

ns winrt
{
    const hstring hexChars = L"0123456789ABCDEF";

    const std::vector<hstring> supportedExts = { L".alac", L".flac", L".m4a", L".mp3", L".wav", L".wma" };

    hstring GetCurrentDateTime(hstring const&);

    double DoubleDivision(double const&, double const&);

    hstring DoubleToFormattedString(double const&, uint32_t const&, wchar_t const&);

    hstring ToHex(long const&);

    Windows::Media::MediaProperties::MediaEncodingProfile GetEncodingProfile(int const&, Windows::Media::MediaProperties::AudioEncodingQuality const&);

    bool CompareFilesByDate(const Windows::Storage::StorageFile&, const Windows::Storage::StorageFile&);

    void SetBinder(
             Windows::UI::Xaml::FrameworkElement const&,
             Windows::UI::Xaml::DependencyProperty const&,
             Windows::Foundation::IInspectable const&,
             Windows::UI::Xaml::Data::BindingMode const& = Windows::UI::Xaml::Data::BindingMode::OneTime,
             Windows::UI::Xaml::Data::IValueConverter const& = nullptr,
             Windows::Foundation::IInspectable const& = nullptr,
             Windows::Foundation::IInspectable const& = nullptr,
             Windows::Foundation::IInspectable const& = nullptr
    );
}