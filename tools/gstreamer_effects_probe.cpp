#include <gst/gst.h>

#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

constexpr std::array<const char *, 6> RequiredFactories = {
    "uridecodebin",
    "audioconvert",
    "audioresample",
    "scaletempo",
    "equalizer-nbands",
    "fakesink",
};

bool requireFactory(const char *name)
{
    GstElementFactory *factory = gst_element_factory_find(name);
    if (factory == nullptr) {
        std::cerr << "Missing GStreamer element factory: " << name << '\n';
        return false;
    }

    gst_object_unref(factory);
    return true;
}

std::string fileUri(const char *path)
{
    gchar *uri = gst_filename_to_uri(path, nullptr);
    if (uri == nullptr) {
        return {};
    }

    std::string result(uri);
    g_free(uri);
    return result;
}

GstElement *makeElement(const char *factoryName, const char *name)
{
    GstElement *element = gst_element_factory_make(factoryName, name);
    if (element == nullptr) {
        std::cerr << "Failed to create GStreamer element: " << factoryName << '\n';
    }
    return element;
}

bool waitForState(GstElement *pipeline, GstState targetState)
{
    const GstStateChangeReturn stateResult = gst_element_set_state(pipeline, targetState);
    if (stateResult == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to move pipeline to requested state.\n";
        return false;
    }

    GstState current = GST_STATE_NULL;
    GstState pending = GST_STATE_NULL;
    const GstStateChangeReturn getStateResult = gst_element_get_state(
        pipeline,
        &current,
        &pending,
        static_cast<GstClockTime>(5 * GST_SECOND));
    if (getStateResult == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Pipeline state transition failed.\n";
        return false;
    }
    if (current != targetState && pending != targetState) {
        std::cerr << "Pipeline did not reach requested state.\n";
        return false;
    }
    return true;
}

bool queryPositionMs(GstElement *pipeline, gint64 &positionMs)
{
    gint64 position = 0;
    if (!gst_element_query_position(pipeline, GST_FORMAT_TIME, &position)) {
        return false;
    }

    positionMs = static_cast<gint64>(position / GST_MSECOND);
    return true;
}

bool seekWithRate(GstElement *pipeline, double rate)
{
    gint64 position = 0;
    gst_element_query_position(pipeline, GST_FORMAT_TIME, &position);

    return gst_element_seek(pipeline,
                            rate,
                            GST_FORMAT_TIME,
                            static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                            GST_SEEK_TYPE_SET,
                            position,
                            GST_SEEK_TYPE_NONE,
                            GST_CLOCK_TIME_NONE);
}

} // namespace

int main(int argc, char **argv)
{
    gst_init(&argc, &argv);

    bool factoriesAvailable = true;
    for (const char *factoryName : RequiredFactories) {
        factoriesAvailable = requireFactory(factoryName) && factoriesAvailable;
    }
    if (!factoriesAvailable) {
        return EXIT_FAILURE;
    }

    if (argc < 2) {
        std::cout << "GStreamer effects factories are available. Pass an audio file path to run pipeline smoke.\n";
        return EXIT_SUCCESS;
    }

    const std::string uri = fileUri(argv[1]);
    if (uri.empty()) {
        std::cerr << "Could not convert input path to file URI.\n";
        return EXIT_FAILURE;
    }

    GstElement *pipeline = gst_pipeline_new("audioplayer-effects-probe");
    GstElement *decode = makeElement("uridecodebin", "decode");
    GstElement *convert = makeElement("audioconvert", "convert");
    GstElement *resample = makeElement("audioresample", "resample");
    GstElement *tempo = makeElement("scaletempo", "tempo");
    GstElement *equalizer = makeElement("equalizer-nbands", "equalizer");
    GstElement *sink = makeElement("fakesink", "sink");

    if (pipeline == nullptr || decode == nullptr || convert == nullptr || resample == nullptr || tempo == nullptr || equalizer == nullptr || sink == nullptr) {
        if (pipeline != nullptr) {
            gst_object_unref(pipeline);
        }
        return EXIT_FAILURE;
    }

    g_object_set(decode, "uri", uri.c_str(), nullptr);
    g_object_set(equalizer, "num-bands", 5U, nullptr);
    g_object_set(sink, "sync", FALSE, nullptr);

    gst_bin_add_many(GST_BIN(pipeline), decode, convert, resample, tempo, equalizer, sink, nullptr);
    if (!gst_element_link_many(convert, resample, tempo, equalizer, sink, nullptr)) {
        std::cerr << "Failed to link audio effects chain.\n";
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    g_signal_connect(decode,
                     "pad-added",
                     G_CALLBACK(+[](GstElement *, GstPad *pad, gpointer userData) {
                         GstElement *target = GST_ELEMENT(userData);
                         GstPad *sinkPad = gst_element_get_static_pad(target, "sink");
                         if (sinkPad == nullptr) {
                             return;
                         }
                         if (!gst_pad_is_linked(sinkPad)) {
                             gst_pad_link(pad, sinkPad);
                         }
                         gst_object_unref(sinkPad);
                     }),
                     convert);

    if (!waitForState(pipeline, GST_STATE_PAUSED)) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    gint64 beforeMs = -1;
    queryPositionMs(pipeline, beforeMs);
    if (!seekWithRate(pipeline, 1.25)) {
        std::cerr << "Failed to apply 1.25x rate-preserving seek segment.\n";
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    if (!waitForState(pipeline, GST_STATE_PLAYING)) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    GstBus *bus = gst_element_get_bus(pipeline);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool ok = true;
    while (std::chrono::steady_clock::now() < deadline) {
        GstMessage *message = gst_bus_timed_pop_filtered(
            bus,
            100 * GST_MSECOND,
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_ASYNC_DONE));
        if (message == nullptr) {
            continue;
        }

        if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
            GError *error = nullptr;
            gchar *debug = nullptr;
            gst_message_parse_error(message, &error, &debug);
            std::cerr << "GStreamer pipeline error: " << (error != nullptr ? error->message : "unknown") << '\n';
            if (debug != nullptr) {
                std::cerr << debug << '\n';
            }
            if (error != nullptr) {
                g_error_free(error);
            }
            g_free(debug);
            ok = false;
        }
        gst_message_unref(message);
        if (!ok) {
            break;
        }
    }

    gint64 afterMs = -1;
    queryPositionMs(pipeline, afterMs);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    if (!ok) {
        return EXIT_FAILURE;
    }

    std::cout << "GStreamer effects pipeline smoke succeeded at 1.25x; position before="
              << beforeMs << "ms after=" << afterMs << "ms\n";
    return EXIT_SUCCESS;
}
