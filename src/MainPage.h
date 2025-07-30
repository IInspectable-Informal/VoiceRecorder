#pragma once
#include "MainPage.g.h"

using ns winrt;
using ns Windows::ApplicationModel;
using ns Windows::Devices::Enumeration;
using ns Windows::Foundation;
using ns Windows::Foundation::Collections;
using ns Windows::Media;
using ns Windows::Media::Audio;
using ns Windows::Media::Capture;
using ns Windows::Media::Core;
using ns Windows::Media::Devices;
using ns Windows::Media::MediaProperties;
using ns Windows::Media::Playback;
using ns Windows::Storage;
using ns Windows::Storage::AccessCache;
using ns Windows::Storage::Pickers;
using ns Windows::Storage::Search;
using ns Windows::Storage::Streams;
using ns Windows::UI;
using ns Windows::UI::Core;
using ns Windows::UI::ViewManagement;
using ns Windows::UI::Xaml;
using ns Windows::UI::Xaml::Controls;
using ns Windows::UI::Xaml::Controls::Primitives;
using ns Windows::UI::Xaml::Data;
using ns Windows::UI::Xaml::Interop;
ns muxc = Microsoft::UI::Xaml::Controls;

using ns StopwatchCore;

ns winrt::VoiceRecorder::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
    public:
        MainPage();

        //XAML binding properties and events
        MediaPlayer Player() { return _Player; }
        //Events
        void ChangeTitleBarTheme(FrameworkElement const&, IInspectable const&);
        void EditSelf(IInspectable const&, IInspectable const&);
        void LoadConfig(IInspectable const&, RoutedEventArgs const&);
        void AppThemeChanged(IInspectable const&, SelectionChangedEventArgs const&);
        fire_and_forget RefreshAudioInputDevices(IInspectable const& = nullptr, RoutedEventArgs const& = nullptr);
        void NotifySelectionChanged(IInspectable const& = nullptr, SelectionChangedEventArgs const& = nullptr);
        void Pause_Resume(IInspectable const&, RoutedEventArgs const&);
        void PlayRecordFile(IInspectable const&, SelectionChangedEventArgs const&);
        fire_and_forget PickFolder(IInspectable const&, RoutedEventArgs const&);
        fire_and_forget Record_Control(IInspectable const&, RoutedEventArgs const&);
        fire_and_forget StopRecord(IInspectable const&, RoutedEventArgs const&);

        //IValueConverter
        IInspectable Convert(IInspectable const&, TypeName const&, IInspectable const&, hstring const&);
        IInspectable ConvertBack(IInspectable const&, TypeName const&, IInspectable const&, hstring const&);

        //Static properties
        static hstring AppName() { return Package::Current().DisplayName(); }
        static Uri AppIcon() { return Package::Current().Logo(); }
        static hstring Developer() { return Package::Current().PublisherDisplayName(); }
        static hstring Version();

        //Destructor
        ~MainPage();
    private:
        Stopwatch RecorderStopwatch;
        FolderPicker DirPicker;
        StorageFolder Folder = nullptr;
        StorageFileQueryResult QueryResult = nullptr; event_token Token;
        InMemoryRandomAccessStream AudioStream; hstring FileName = L"";
        std::map<hstring, MediaCapture> Recorders;
        MediaPlayer _Player; SystemMediaTransportControlsDisplayUpdater Updater = nullptr;
        ApplicationViewTitleBar TitleBar = ApplicationView::GetForCurrentView().TitleBar();
        IObservableVector<DeviceInformation> _Devices = single_threaded_observable_vector<DeviceInformation>();
        IObservableVector<StorageFile> _Records = single_threaded_observable_vector<StorageFile>(); int index = -1;

        void ChangeThemeText(int const&);
        fire_and_forget SetTracker(); bool IsSameFolder = true;
        fire_and_forget Tracker(IStorageQueryResultBase const&, IInspectable const&); bool IsTracking = false;
        void SetSelectorsAvalibility(bool const&);
        void ShowInfo(muxc::InfoBarSeverity const&, hstring const&);
        fire_and_forget Looper(Stopwatch const&, IInspectable const&);
        fire_and_forget PrepareSMTC(MediaPlayer const&, IInspectable const&);
    };
}

ns winrt::VoiceRecorder::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
