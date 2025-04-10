#pragma once
#include "App.xaml.g.h"
#include "public.h"

using ns winrt;
using ns Windows::ApplicationModel;
using ns Windows::ApplicationModel::Activation;
using ns Windows::ApplicationModel::Core;
using ns Windows::Foundation;
using ns Windows::Storage;
using ns Windows::UI;
using ns Windows::UI::Core;
using ns Windows::UI::ViewManagement;
using ns Windows::UI::Xaml;
using ns Windows::UI::Xaml::Controls;

ns winrt::VoiceRecorder::implementation
{
    struct App : AppT<App>
    {
        App();
        void OnLaunched(LaunchActivatedEventArgs const&);
        void OnSuspending(IInspectable const&, SuspendingEventArgs const&);
    private:
        bool Inited = false;
        fire_and_forget CreateView();
        int InitView();
    };
}
