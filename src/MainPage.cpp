#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include "helpers.h"
#include "public.h"

//STL
#include <cwctype>

namespace winrt::VoiceRecorder::implementation
{
    MainPage::MainPage()
    {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        RecorderStopwatch.Working({ this, &MainPage::Looper });
        RecorderStopwatch.Interval(std::chrono::milliseconds(8));
        DirPicker.SuggestedStartLocation(PickerLocationId::MusicLibrary);
        DirPicker.FileTypeFilter().Append(L"*");
        Updater = _Player.SystemMediaTransportControls().DisplayUpdater();
        _Player.MediaOpened({ this, &MainPage::PrepareSMTC });
        _Player.MediaFailed([this](auto const&, auto const& args)
        {
            auto argsCopy = args.as<MediaPlayerFailedEventArgs>();
            auto errorMessage = argsCopy.ErrorMessage();
            Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [this, errorMessage]()
            {
                ShowInfo(muxc::InfoBarSeverity::Error, errorMessage);
                RecordsList().IsEnabled(true);
            });
        });
    }

    //Xaml binding properties and events

    //Events
    void MainPage::ChangeTitleBarTheme(FrameworkElement const& sender, IInspectable const&)
    {
        if (sender.ActualTheme() == ElementTheme::Light)
        { TitleBar.ButtonForegroundColor(Colors::Black()); TitleBar.ButtonInactiveForegroundColor(Colors::DarkGray()); }
        else { TitleBar.ButtonForegroundColor(Colors::White()); TitleBar.ButtonInactiveForegroundColor(Colors::LightGray()); }
        //Dlg.RequestedTheme(sender.ActualTheme());
    }

    void MainPage::EditSelf(IInspectable const&, IInspectable const&)
    {
        auto const& mediaTC = PlayerUI().TransportControls();
        if (mediaTC.IsLoaded())
        {
            auto const& button = mediaTC.as<IControlProtected>().GetTemplateChild(L"CastButton");
            if (button)
            { button.as<AppBarButton>().Visibility(Visibility::Collapsed); }
        }
    }

    void MainPage::LoadConfig(IInspectable const& sender, RoutedEventArgs const&)
    {
        PlayerUI().SetMediaPlayer(_Player);
        //auto const& mediaTC = PlayerUI().TransportControls();
        int const& i2 = (unbox_value<int>(AppDC.Values().Lookup(L"AppTheme")) + 2) % 3;
        ChangeThemeText(i2); sender.as<muxc::Expander>().Content().as<muxc::RadioButtons>().SelectedIndex(i2);
        IPropertySet const& propSet = AppDC.Values();
        if (propSet.HasKey(L"AudioQuality"))
        { AudioQuality().SelectedIndex(unbox_value<int>(propSet.Lookup(L"AudioQuality"))); }
        if (propSet.HasKey(L"FileFormat"))
        { Formats().SelectedIndex(unbox_value<int>(propSet.Lookup(L"FileFormat"))); }
        RefreshAudioInputDevices();
        SelectedDevice().SelectionChanged({ this, &MainPage::NotifySelectionChanged });
        AudioQuality().SelectionChanged({ this, &MainPage::NotifySelectionChanged });
        Formats().SelectionChanged({ this, &MainPage::NotifySelectionChanged });
        SelectedDevice().ItemsSource(_Devices); RecordsList().ItemsSource(_Records);
        if (!propSet.HasKey(L"FolderToken"))
        {
            try
            { Folder = StorageLibrary::GetLibraryAsync(KnownLibraryId::Music).get().SaveFolder(); }
            catch ([[maybe_unused]] hresult_error const& ex)
            {
                Folder = ApplicationData::Current().LocalFolder();
                ShowInfo(muxc::InfoBarSeverity::Error, L"本应用无法获取音乐库，因此只能将录音文件保存到应用程序数据目录。你可以手动完成这个操作。\nHRESULT: 0x" + ToHex(ex.code()));
            } propSet.Insert(L"FolderToken", box_value(FA.Add(Folder)));
        } else { Folder = FA.GetFolderAsync(unbox_value<hstring>(propSet.Lookup(L"FolderToken"))).get(); }
        SetTracker(); PathDisplayer().Text(Folder.Path());
    }

    void MainPage::AppThemeChanged(IInspectable const& sender, SelectionChangedEventArgs const&)
    {
        muxc::RadioButtons const& control = sender.as<muxc::RadioButtons>(); int const& AppTheme = control.SelectedIndex();
        Frame().RequestedTheme(static_cast<ElementTheme>((AppTheme + 1) % 3)); ChangeThemeText(AppTheme);
        AppDC.Values().Insert(L"AppTheme", box_value((AppTheme + 1) % 3));
    }

    fire_and_forget MainPage::RefreshAudioInputDevices(IInspectable const&, RoutedEventArgs const&)
    {
        RecordButton().IsEnabled(false);
        SelectedDevice().IsEnabled(false);
        SelectedDevice().PlaceholderText(L"正在获取可用设备");
        SelectedDevice().SelectedIndex(-1);
        _Devices.Clear();
        DeviceInformationCollection const& devs = co_await DeviceInformation::FindAllAsync(MediaDevice::GetAudioCaptureSelector());
        uint32_t const& size = devs.Size(); IPropertySet const& propSet = AppDC.Values();
        hstring const& devId = propSet.HasKey(L"DeviceId") ? propSet.Lookup(L"DeviceId").as<hstring>() : L"";
        if (size > 0)
        {
            for (uint32_t i = 0; i < size; ++i)
            {
                DeviceInformation const& dev = devs.GetAt(i);
                _Devices.Append(dev);
                if (dev.Id() == devId)
                { SelectedDevice().SelectedIndex(i); }
            } SelectedDevice().PlaceholderText(L"请选择麦克风");
        } else { SelectedDevice().PlaceholderText(L"没有可用的麦克风"); }
        SelectedDevice().IsEnabled(true);
    }

    void MainPage::NotifySelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        int const& devId = SelectedDevice().SelectedIndex(); int const& qualityId = AudioQuality().SelectedIndex(); int const& formatId = Formats().SelectedIndex();
        if (devId == -1 || qualityId == -1 || formatId == -1)
        { RecordButton().IsEnabled(false); }
        else { RecordButton().IsEnabled(true); }
        IPropertySet const& propSet = AppDC.Values();
        if (devId == -1) { propSet.Remove(L"DeviceId"); }
        else { propSet.Insert(L"DeviceId", box_value(_Devices.GetAt(devId).Id())); }
        propSet.Insert(L"AudioQuality", box_value(qualityId));
        propSet.Insert(L"FileFormat", box_value(formatId));
    }

    void MainPage::Pause_Resume(IInspectable const&, RoutedEventArgs const&)
    {
        if (_Player.PlaybackSession().PlaybackState() == MediaPlaybackState::Playing)
        { _Player.Pause(); }
        else { _Player.Play(); }
    }

    void MainPage::PlayRecordFile(IInspectable const&, SelectionChangedEventArgs const&)
    {
        if (!IsTracking && RecordsList().SelectedIndex() != index)
        {
            RecordsList().IsEnabled(false); Info().IsOpen(false); _Player.Pause(); TotalTimeDisplayer().Text(L"--:--:--");

            index = RecordsList().SelectedIndex();
            if (index != -1)
            { _Player.Source(MediaSource::CreateFromStream(_Records.GetAt(index).OpenAsync(FileAccessMode::Read).get(), _Records.GetAt(index).ContentType())); }
            else
            {
                Updater.ClearAll(); Updater.Update(); _Player.Source(nullptr);
                NameDisplayer().Text(L"（未选择）"); RecordsList().IsEnabled(true);
            }
        }
    }

    fire_and_forget MainPage::PickFolder(IInspectable const&, RoutedEventArgs const&)
    {
        PickButton().IsEnabled(false);
        auto dir = co_await DirPicker.PickSingleFolderAsync();
        if (dir && dir.Path() != Folder.Path())
        {
            IsSameFolder = false; QueryResult.ContentsChanged(Token); Folder = dir;
            FA.Clear(); AppDC.Values().Insert(L"FolderToken", box_value(FA.Add(dir)));
            PathDisplayer().Text(Folder.Path());
            SetTracker();
        } dir = nullptr; PickButton().IsEnabled(true);
    }

    fire_and_forget MainPage::Record_Control(IInspectable const&, RoutedEventArgs const&)
    {
        RecordButton().IsEnabled(false);
        Info().IsOpen(false); Info().Message(L"");
        LoudnessDisplayer().IsIndeterminate(true);
        hstring const& deviceId = _Devices.GetAt(SelectedDevice().SelectedIndex()).Id();
        switch (static_cast<wchar_t>(RecordButton().Content().as<hstring>()[0]))
        {
            case L'\xE7C8':
            {
                SetSelectorsAvalibility(false);
                MediaCaptureInitializationSettings settings = nullptr;
                MediaCapture recorder = nullptr;
                try
                {
                    if (Recorders.find(deviceId) == Recorders.end())
                    {
                        settings = MediaCaptureInitializationSettings(); recorder = MediaCapture();
                        settings.StreamingCaptureMode(StreamingCaptureMode::Audio);
                        settings.AudioDeviceId(deviceId);
                        co_await recorder.InitializeAsync(settings);
                        Recorders.emplace(deviceId, recorder);
                    }
                    RecordButton().Content(box_value(L"\xF8AE"));
                    FileName = L"Record_" + GetCurrentDateTime(L"{year.full}-{month.integer(2)}-{day.integer(2)}_{hour.integer(2)}-{minute.integer(2)}-{second.integer(2)}")
                               + supportedExts.at(Formats().SelectedIndex());
                    AudioStream = InMemoryRandomAccessStream();
                    co_await Recorders.at(deviceId).StartRecordToStreamAsync(
                                 GetEncodingProfile(Formats().SelectedIndex(), static_cast<AudioEncodingQuality>(AudioQuality().SelectedIndex())),
                                 AudioStream
                             );
                    RecorderStopwatch.Start();
                    StopButton().IsEnabled(true);
                    ToolTipService::SetToolTip(RecordButton(), box_value(L"暂停"));
                }
                catch (hresult_error const& ex)
                {
                    ShowInfo(muxc::InfoBarSeverity::Error, ex.message() + L"\nHRESULT: 0x" + ToHex(ex.code()));
                    SetSelectorsAvalibility(true); recorder = nullptr;
                }
                catch (std::exception const& ex)
                {
                    ShowInfo(muxc::InfoBarSeverity::Error, to_hstring(ex.what()));
                    SetSelectorsAvalibility(true); recorder = nullptr;
                } settings = nullptr; break;
            }
            case L'\xF8AE':
            {
                co_await Recorders[deviceId].PauseRecordAsync(MediaCapturePauseBehavior::ReleaseHardwareResources);
                RecorderStopwatch.Pause();
                RecordButton().Content(box_value(L"\xF5B0"));
                ToolTipService::SetToolTip(RecordButton(), box_value(L"继续"));
                break;
            }
            case L'\xF5B0':
            {
                co_await Recorders[deviceId].ResumeRecordAsync();
                RecorderStopwatch.Resume();
                RecordButton().Content(box_value(L"\xF8AE"));
                ToolTipService::SetToolTip(RecordButton(), box_value(L"暂停"));
                break;
            }
        }
        LoudnessDisplayer().IsIndeterminate(false);
        RecordButton().IsEnabled(true);
    }

    fire_and_forget MainPage::StopRecord(IInspectable const&, RoutedEventArgs const&)
    {
        StopButton().IsEnabled(false); LoudnessDisplayer().IsIndeterminate(true);
        try
        {
            co_await Recorders[_Devices.GetAt(SelectedDevice().SelectedIndex()).Id()].StopRecordAsync();
            RecorderStopwatch.Reset();
            auto fileStream = co_await(co_await Folder.CreateFileAsync(FileName, CreationCollisionOption::GenerateUniqueName)).OpenAsync(FileAccessMode::ReadWrite);
            AudioStream.Seek(0); fileStream.Seek(0);
            co_await RandomAccessStream::CopyAndCloseAsync(AudioStream, fileStream);
            AudioStream.Close(); fileStream = nullptr; AudioStream = nullptr;
            ShowInfo(muxc::InfoBarSeverity::Success, L"录音已成功保存！");
        }
        catch (hresult_error const& ex)
        { ShowInfo(muxc::InfoBarSeverity::Error, L"发生错误：" + ex.message() + L"\nHRESULT: 0x" + ToHex(ex.code())); }
        catch (...)
        { ShowInfo(muxc::InfoBarSeverity::Error, L"发生了未知错误\nHRESULT: 0x" + ToHex(to_hresult())); }
        LoudnessDisplayer().IsIndeterminate(false);
        DurationDisplayer().Text(L"00 : 00 : 00 . 00");
        SetSelectorsAvalibility(true);
        RecordButton().Content(box_value(L"\xE7C8"));
        ToolTipService::SetToolTip(RecordButton(), box_value(L"录音"));
    }

    //IValueConverter
    IInspectable MainPage::Convert(IInspectable const& value, TypeName const& targetType, IInspectable const& param, hstring const&)
    {
        hstring const& runtimeClassName = get_class_name(value);
        if (runtimeClassName == L"Windows.Foundation.TimeSpan")
        {
            TimeSpan const& duration = value.as<TimeSpan>();
            if (targetType == xaml_typename<double>())
            {
                double ms = -1;
                if (param.as<hstring>() == L"FromPlayer")
                {
                    Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [&ms, &duration]()
                    { ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(duration).count(); }).get();
                } return box_value(ms);
            }
        } return nullptr;
    }

    IInspectable MainPage::ConvertBack(IInspectable const& value, TypeName const& targetType, IInspectable const&, hstring const&)
    {
        hstring const& runtimeClassName = get_class_name(value);
        if (runtimeClassName == L"Double")
        {
            double const& duration = value.as<double>();
            if (targetType == xaml_typename<TimeSpan>())
            { return box_value(std::chrono::duration_cast<TimeSpan>(std::chrono::duration<double>(duration / 1000))); }
        } return nullptr;
    }

    //Private functions
    void MainPage::ChangeThemeText(int const& AppTheme)
    {
        hstring ThemeText = L"";
        switch (AppTheme)
        {
            case 0: ThemeText = L"浅色"; break;
            case 1: ThemeText = L"深色"; break;
            case 2: ThemeText = L"跟随系统"; break;
        } CurAppThemeDisplay().Text(ThemeText);
    }

    fire_and_forget MainPage::SetTracker()
    {
        QueryOptions options;
        options.FolderDepth(FolderDepth::Shallow);
        QueryResult = Folder.CreateFileQueryWithOptions(options);
        Token = QueryResult.ContentsChanged({ this, &MainPage::Tracker });
        co_await QueryResult.GetFilesAsync();
        options = nullptr;
    }

    fire_and_forget MainPage::Tracker(IStorageQueryResultBase const&, IInspectable const&)
    {
        co_await Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [this]()
        {
            IsTracking = true;
            _Records.Clear();
            std::vector<StorageFile> files;
            IVectorView<StorageFile> const& filesv = Folder.GetFilesAsync().get();
            for (auto const& file : filesv)
            {
                hstring ext = file.FileType();
                std::transform(ext.begin(), ext.end(), const_cast<hstring::pointer>(ext.begin()), [](wchar_t c) { return std::towlower(c); });
                if (std::find(supportedExts.begin(), supportedExts.end(), ext) != supportedExts.end())
                { files.push_back(file); }
            }
            std::sort(files.begin(), files.end(), CompareFilesByDate);
            for (uint32_t i = 0; i < files.size(); ++i)
            {
                _Records.Append(files.at(i));
                if (IsSameFolder && files.at(i).Name() == NameDisplayer().Text())
                { index = i; RecordsList().SelectedIndex(i); }
            } IsSameFolder = true;
            files.clear();
            IsTracking = false;
        });
    }

    void MainPage::SetSelectorsAvalibility(bool const& value)
    {
        SelectedDevice().IsEnabled(value);
        AudioQuality().IsEnabled(value);
        Formats().IsEnabled(value);
        PickButton().IsEnabled(value);
    }

    void MainPage::ShowInfo(muxc::InfoBarSeverity const& severity, hstring const& message)
    {
        Info().Severity(severity);
        Info().Message(message);
        Info().IsOpen(true);
    }

    void MainPage::Looper(Stopwatch const& sender, IInspectable const&)
    {
        auto tms = sender.Duration();
        auto hh = DoubleDivision(tms, (1000 * 60 * 60));
        tms -= (1000 * 60 * 60 * hh);
        auto mm = DoubleDivision(tms, (1000 * 60));
        tms -= (1000 * 60 * mm);
        auto ss = DoubleDivision(tms, 1000);
        auto ms = DoubleDivision(tms - 1000 * ss, 10);
        DurationDisplayer().Text(DoubleToFormattedString(hh, 2, L'0') + L" : " + 
                                 DoubleToFormattedString(mm, 2, L'0') + L" : " + 
                                 DoubleToFormattedString(ss, 2, L'0') + L" . " + 
                                 DoubleToFormattedString(ms, 2, L'0'));
    }

    fire_and_forget MainPage::PrepareSMTC(MediaPlayer const&, IInspectable const&)
    {
        _Player.PlaybackSession().Position(TimeSpan(0));
        co_await Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [this]()
        {
            if (_Player.PlaybackSession().NaturalDuration().count() > 0)
            {
                int64_t totalSeconds = _Player.PlaybackSession().NaturalDuration().count() / 10000000;

                int hh = static_cast<int>(totalSeconds / 3600);
                int mm = static_cast<int>((totalSeconds % 3600) / 60);
                int ss = static_cast<int>(totalSeconds % 60);

                TotalTimeDisplayer().Text(DoubleToFormattedString(hh, 2, L'0') + L":" + 
                                          DoubleToFormattedString(mm, 2, L'0') + L":" + 
                                          DoubleToFormattedString(ss, 2, L'0'));
            }
            else
            {
                TotalTimeDisplayer().Text(L"00:00:00"); // 默认值
            }
            Updater.Type(MediaPlaybackType::Music);
            Updater.MusicProperties().Title(_Records.GetAt(index).Name());
            Updater.Update(); 
            NameDisplayer().Text(_Records.GetAt(index).Name());
            RecordsList().IsEnabled(true);
        }); _Player.Play();
    }

    //Static properties
    hstring MainPage::Version()
    {
        PackageVersion const& Ver = Package::Current().Id().Version();
        return to_hstring(Ver.Major) + L"." + to_hstring(Ver.Minor) + L"." + to_hstring(Ver.Build) + L"." + to_hstring(Ver.Revision);
    }

    //Destructor
    MainPage::~MainPage()
    {
        for (auto const& item : Recorders)
        { item.second.Close(); }
        Recorders.clear();
        _Player.Close();
    }
}
