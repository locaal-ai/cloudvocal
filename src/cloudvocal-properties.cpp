
#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>

#include <string>
#include <vector>

#include "cloudvocal-data.h"
#include "cloudvocal.h"
#include "cloudvocal-utils.h"
#include "cloud-translation/language-codes.h"
#include "plugin-support.h"
// #include "ui/filter-replace-dialog.h"
// #include "ui/filter-replace-utils.h"

bool translation_options_callback(obs_properties_t *props, obs_property_t *property,
				  obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	// Show/Hide the translation group
	const bool translate_enabled = obs_data_get_bool(settings, "translate");
	const bool is_advanced = obs_data_get_int(settings, "advanced_settings_mode") == 1;
	for (const auto &prop :
	     {"translate_target_language", "translate_model", "translate_output"}) {
		obs_property_set_visible(obs_properties_get(props, prop), translate_enabled);
	}
	for (const auto &prop :
	     {"translate_add_context", "translate_input_tokenization_style",
	      "translation_sampling_temperature", "translation_repetition_penalty",
	      "translation_beam_size", "translation_max_decoding_length",
	      "translation_no_repeat_ngram_size", "translation_max_input_length",
	      "translate_only_full_sentences"}) {
		obs_property_set_visible(obs_properties_get(props, prop),
					 translate_enabled && is_advanced);
	}
	const bool is_external =
		(strcmp(obs_data_get_string(settings, "translate_model"), "!!!external!!!") == 0);
	obs_property_set_visible(obs_properties_get(props, "translation_model_path_external"),
				 is_external && translate_enabled);
	return true;
}

bool translation_cloud_provider_selection_callback(obs_properties_t *props, obs_property_t *p,
						   obs_data_t *s)
{
	UNUSED_PARAMETER(p);
	const char *provider = obs_data_get_string(s, "translate_cloud_provider");
	// show the access key for all except the custom provider
	obs_property_set_visible(obs_properties_get(props, "translate_cloud_api_key"),
				 strcmp(provider, "api") != 0);
	obs_property_set_visible(obs_properties_get(props, "translate_cloud_deepl_free"),
				 strcmp(provider, "deepl") == 0);
	// show the secret key input for the papago provider only
	obs_property_set_visible(obs_properties_get(props, "translate_cloud_secret_key"),
				 strcmp(provider, "papago") == 0);
	// show the region input for the azure provider only
	obs_property_set_visible(obs_properties_get(props, "translate_cloud_region"),
				 strcmp(provider, "azure") == 0);
	// show the endpoint and body input for the custom provider only
	obs_property_set_visible(obs_properties_get(props, "translate_cloud_endpoint"),
				 strcmp(provider, "api") == 0);
	obs_property_set_visible(obs_properties_get(props, "translate_cloud_body"),
				 strcmp(provider, "api") == 0);
	// show the response json path input for the custom provider only
	obs_property_set_visible(obs_properties_get(props, "translate_cloud_response_json_path"),
				 strcmp(provider, "api") == 0);
	return true;
}

bool translation_cloud_options_callback(obs_properties_t *props, obs_property_t *property,
					obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	// Show/Hide the cloud translation group options
	const bool translate_enabled = obs_data_get_bool(settings, "translate_cloud");
	for (const auto &prop :
	     {"translate_cloud_provider", "translate_cloud_target_language",
	      "translate_cloud_output", "translate_cloud_api_key",
	      "translate_cloud_only_full_sentences", "translate_cloud_secret_key",
	      "translate_cloud_deepl_free", "translate_cloud_region", "translate_cloud_endpoint",
	      "translate_cloud_body", "translate_cloud_response_json_path"}) {
		obs_property_set_visible(obs_properties_get(props, prop), translate_enabled);
	}
	if (translate_enabled) {
		translation_cloud_provider_selection_callback(props, NULL, settings);
	}
	return true;
}

bool advanced_settings_callback(obs_properties_t *props, obs_property_t *property,
				obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	// If advanced settings is enabled, show the advanced settings group
	const bool show_hide = obs_data_get_int(settings, "advanced_settings_mode") == 1;
	for (const std::string &prop_name :
	     {"whisper_params_group", "buffered_output_group", "log_group", "advanced_group",
	      "file_output_enable", "partial_group"}) {
		obs_property_set_visible(obs_properties_get(props, prop_name.c_str()), show_hide);
	}
	translation_options_callback(props, NULL, settings);
	translation_cloud_options_callback(props, NULL, settings);
	return true;
}

bool file_output_select_changed(obs_properties_t *props, obs_property_t *property,
				obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	// Show or hide the output filename selection input
	const bool show_hide = obs_data_get_bool(settings, "file_output_enable");
	for (const std::string &prop_name :
	     {"subtitle_output_filename", "subtitle_save_srt", "truncate_output_file",
	      "only_while_recording", "rename_file_to_match_recording", "file_output_info"}) {
		obs_property_set_visible(obs_properties_get(props, prop_name.c_str()), show_hide);
	}
	return true;
}

void add_translation_cloud_group_properties(obs_properties_t *ppts)
{
	// add translation cloud group
	obs_properties_t *translation_cloud_group = obs_properties_create();
	obs_property_t *translation_cloud_group_prop =
		obs_properties_add_group(ppts, "translate_cloud", MT_("translate_cloud"),
					 OBS_GROUP_CHECKABLE, translation_cloud_group);

	obs_property_set_modified_callback(translation_cloud_group_prop,
					   translation_cloud_options_callback);

	// add explaination text
	obs_properties_add_text(translation_cloud_group, "translate_cloud_explaination",
				MT_("translate_cloud_explaination"), OBS_TEXT_INFO);

	// add cloud translation service provider selection
	obs_property_t *prop_translate_cloud_provider = obs_properties_add_list(
		translation_cloud_group, "translate_cloud_provider",
		MT_("translate_cloud_provider"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	// Populate the dropdown with the cloud translation service providers
	obs_property_list_add_string(prop_translate_cloud_provider, MT_("Google-Cloud-Translation"),
				     "google");
	obs_property_list_add_string(prop_translate_cloud_provider, MT_("Microsoft-Translator"),
				     "azure");
	// obs_property_list_add_string(prop_translate_cloud_provider, MT_("Amazon-Translate"),
	// 			     "amazon-translate");
	// obs_property_list_add_string(prop_translate_cloud_provider, MT_("IBM-Watson-Translate"),
	// 			     "ibm-watson-translate");
	// obs_property_list_add_string(prop_translate_cloud_provider, MT_("Yandex-Translate"),
	// 			     "yandex-translate");
	// obs_property_list_add_string(prop_translate_cloud_provider, MT_("Baidu-Translate"),
	// 			     "baidu-translate");
	// obs_property_list_add_string(prop_translate_cloud_provider, MT_("Tencent-Translate"),
	// 			     "tencent-translate");
	// obs_property_list_add_string(prop_translate_cloud_provider, MT_("Alibaba-Translate"),
	// 			     "alibaba-translate");
	// obs_property_list_add_string(prop_translate_cloud_provider, MT_("Naver-Translate"),
	// 			     "naver-translate");
	// obs_property_list_add_string(prop_translate_cloud_provider, MT_("Kakao-Translate"),
	// 			     "kakao-translate");
	obs_property_list_add_string(prop_translate_cloud_provider, MT_("Papago-Translate"),
				     "papago");
	obs_property_list_add_string(prop_translate_cloud_provider, MT_("Deepl-Translate"),
				     "deepl");
	obs_property_list_add_string(prop_translate_cloud_provider, MT_("OpenAI-Translate"),
				     "openai");
	obs_property_list_add_string(prop_translate_cloud_provider, MT_("Claude-Translate"),
				     "claude");
	obs_property_list_add_string(prop_translate_cloud_provider, MT_("API-Translate"), "api");

	// add callback to show/hide the free API option for deepl
	obs_property_set_modified_callback(prop_translate_cloud_provider,
					   translation_cloud_provider_selection_callback);

	// add target language selection
	obs_property_t *prop_tgt = obs_properties_add_list(
		translation_cloud_group, "translate_cloud_target_language", MT_("target_language"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	// Populate the dropdown with the language codes
	for (const auto &language : language_codes) {
		obs_property_list_add_string(prop_tgt, language.second.c_str(),
					     language.first.c_str());
	}
	// add option for routing the translation to an output source
	obs_property_t *prop_output = obs_properties_add_list(
		translation_cloud_group, "translate_cloud_output", MT_("translate_output"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(prop_output, "Write to captions output", "none");
	obs_enum_sources(add_sources_to_list, prop_output);

	// add boolean option for only full sentences
	obs_properties_add_bool(translation_cloud_group, "translate_cloud_only_full_sentences",
				MT_("translate_cloud_only_full_sentences"));

	// add input for API Key
	obs_properties_add_text(translation_cloud_group, "translate_cloud_api_key",
				MT_("translate_cloud_api_key"), OBS_TEXT_DEFAULT);
	// add input for secret key
	obs_properties_add_text(translation_cloud_group, "translate_cloud_secret_key",
				MT_("translate_cloud_secret_key"), OBS_TEXT_PASSWORD);

	// add boolean option for free API from deepl
	obs_properties_add_bool(translation_cloud_group, "translate_cloud_deepl_free",
				MT_("translate_cloud_deepl_free"));

	// add translate_cloud_region for azure
	obs_properties_add_text(translation_cloud_group, "translate_cloud_region",
				MT_("translate_cloud_region"), OBS_TEXT_DEFAULT);

	// add input for API endpoint
	obs_properties_add_text(translation_cloud_group, "translate_cloud_endpoint",
				MT_("translate_cloud_endpoint"), OBS_TEXT_DEFAULT);
	// add input for API body
	obs_properties_add_text(translation_cloud_group, "translate_cloud_body",
				MT_("translate_cloud_body"), OBS_TEXT_MULTILINE);
	// add input for json response path
	obs_properties_add_text(translation_cloud_group, "translate_cloud_response_json_path",
				MT_("translate_cloud_response_json_path"), OBS_TEXT_DEFAULT);
}

void add_file_output_group_properties(obs_properties_t *ppts)
{
	// create a file output group
	obs_properties_t *file_output_group = obs_properties_create();
	// add a checkbox group for file output
	obs_property_t *file_output_group_prop =
		obs_properties_add_group(ppts, "file_output_enable", MT_("file_output_group"),
					 OBS_GROUP_CHECKABLE, file_output_group);

	obs_properties_add_path(file_output_group, "subtitle_output_filename",
				MT_("output_filename"), OBS_PATH_FILE_SAVE, "Text (*.txt)", NULL);
	// add info text about the file output
	obs_properties_add_text(file_output_group, "file_output_info", MT_("file_output_info"),
				OBS_TEXT_INFO);
	obs_properties_add_bool(file_output_group, "subtitle_save_srt", MT_("save_srt"));
	obs_properties_add_bool(file_output_group, "truncate_output_file",
				MT_("truncate_output_file"));
	obs_properties_add_bool(file_output_group, "only_while_recording",
				MT_("only_while_recording"));
	obs_properties_add_bool(file_output_group, "rename_file_to_match_recording",
				MT_("rename_file_to_match_recording"));
	obs_property_set_modified_callback(file_output_group_prop, file_output_select_changed);
}

void add_advanced_group_properties(obs_properties_t *ppts, struct cloudvocal_data *gf)
{
	// add a group for advanced configuration
	obs_properties_t *advanced_config_group = obs_properties_create();
	obs_properties_add_group(ppts, "advanced_group", MT_("advanced_group"), OBS_GROUP_NORMAL,
				 advanced_config_group);

	obs_properties_add_bool(advanced_config_group, "caption_to_stream",
				MT_("caption_to_stream"));

	obs_properties_add_int_slider(advanced_config_group, "min_sub_duration",
				      MT_("min_sub_duration"), 1000, 5000, 50);
	obs_properties_add_int_slider(advanced_config_group, "max_sub_duration",
				      MT_("max_sub_duration"), 1000, 5000, 50);
	obs_properties_add_float_slider(advanced_config_group, "sentence_psum_accept_thresh",
					MT_("sentence_psum_accept_thresh"), 0.0, 1.0, 0.05);

	obs_properties_add_bool(advanced_config_group, "process_while_muted",
				MT_("process_while_muted"));

	// add button to open filter and replace UI dialog
	// obs_properties_add_button2(
	// 	advanced_config_group, "open_filter_ui", MT_("open_filter_ui"),
	// 	[](obs_properties_t *props, obs_property_t *property, void *data_) {
	// 		UNUSED_PARAMETER(props);
	// 		UNUSED_PARAMETER(property);
	// 		struct cloudvocal_data *gf_ =
	// 			static_cast<struct cloudvocal_data *>(data_);
	// 		FilterReplaceDialog *filter_replace_dialog = new FilterReplaceDialog(
	// 			(QWidget *)obs_frontend_get_main_window(), gf_);
	// 		filter_replace_dialog->exec();
	// 		// store the filter data on the source settings
	// 		obs_data_t *settings = obs_source_get_settings(gf_->context);
	// 		// serialize the filter data
	// 		const std::string filter_data =
	// 			serialize_filter_words_replace(gf_->filter_words_replace);
	// 		obs_data_set_string(settings, "filter_words_replace", filter_data.c_str());
	// 		obs_data_release(settings);
	// 		return true;
	// 	},
	// 	gf);
}

void add_logging_group_properties(obs_properties_t *ppts)
{
	// add a group for Logging options
	obs_properties_t *log_group = obs_properties_create();
	obs_properties_add_group(ppts, "log_group", MT_("log_group"), OBS_GROUP_NORMAL, log_group);

	obs_properties_add_bool(log_group, "log_words", MT_("log_words"));
	obs_property_t *list = obs_properties_add_list(log_group, "log_level", MT_("log_level"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, "DEBUG (Won't show)", LOG_DEBUG);
	obs_property_list_add_int(list, "INFO", LOG_INFO);
	obs_property_list_add_int(list, "WARNING", LOG_WARNING);
}

void add_general_group_properties(obs_properties_t *ppts)
{
	// add "General" group
	obs_properties_t *general_group = obs_properties_create();
	obs_properties_add_group(ppts, "general_group", MT_("general_group"), OBS_GROUP_NORMAL,
				 general_group);

	obs_property_t *subs_output =
		obs_properties_add_list(general_group, "subtitle_sources", MT_("subtitle_sources"),
					OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	// Add "none" option
	obs_property_list_add_string(subs_output, MT_("none_no_output"), "none");
	// Add text sources
	obs_enum_sources(add_sources_to_list, subs_output);

	// Add language selector
	obs_property_t *whisper_language_select_list =
		obs_properties_add_list(general_group, "whisper_language_select", MT_("language"),
					OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	// iterate over all available languages and add them to the list
	for (auto const &pair : language_codes_reverse) {
		obs_property_list_add_string(whisper_language_select_list, pair.first.c_str(),
					     pair.second.c_str());
	}
}

void add_partial_group_properties(obs_properties_t *ppts)
{
	// add a group for partial transcription
	obs_properties_t *partial_group = obs_properties_create();
	obs_properties_add_group(ppts, "partial_group", MT_("partial_transcription"),
				 OBS_GROUP_CHECKABLE, partial_group);

	// add text info
	obs_properties_add_text(partial_group, "partial_info", MT_("partial_transcription_info"),
				OBS_TEXT_INFO);

	// add slider for partial latecy
	obs_properties_add_int_slider(partial_group, "partial_latency", MT_("partial_latency"), 500,
				      3000, 50);
}

obs_properties_t *cloudvocal_properties(void *data)
{
	struct cloudvocal_data *gf = static_cast<struct cloudvocal_data *>(data);

	obs_properties_t *ppts = obs_properties_create();

	// add a drop down selection for advanced vs simple settings
	obs_property_t *advanced_settings = obs_properties_add_list(ppts, "advanced_settings_mode",
								    MT_("advanced_settings_mode"),
								    OBS_COMBO_TYPE_LIST,
								    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(advanced_settings, MT_("simple_mode"), 0);
	obs_property_list_add_int(advanced_settings, MT_("advanced_mode"), 1);
	obs_property_set_modified_callback(advanced_settings, advanced_settings_callback);

	add_general_group_properties(ppts);
	add_translation_cloud_group_properties(ppts);
	add_file_output_group_properties(ppts);
	add_advanced_group_properties(ppts, gf);
	add_logging_group_properties(ppts);
	add_partial_group_properties(ppts);

	// Add a informative text about the plugin
	const char *template_out = PLUGIN_INFO_TEMPLATE;
	std::string version_str = PLUGIN_VERSION;
	size_t pos = std::string(template_out).find("{{plugin_version}}");
	if (pos != std::string::npos) {
		template_out = std::string(template_out).replace(pos, 17, version_str).c_str();
	}
	obs_properties_add_text(ppts, "info", template_out, OBS_TEXT_INFO);

	UNUSED_PARAMETER(data);
	return ppts;
}

void cloudvocal_defaults(obs_data_t *s)
{
	obs_log(LOG_DEBUG, "filter defaults");

	obs_data_set_default_double(s, "duration_filter_threshold", 2.25);
	obs_data_set_default_int(s, "segment_duration", 7000);
	obs_data_set_default_int(s, "log_level", LOG_DEBUG);
	obs_data_set_default_bool(s, "log_words", false);
	obs_data_set_default_bool(s, "caption_to_stream", false);
	obs_data_set_default_string(s, "whisper_model_path", "Whisper Tiny English (74Mb)");
	obs_data_set_default_string(s, "whisper_language_select", "en");
	obs_data_set_default_string(s, "subtitle_sources", "none");
	obs_data_set_default_bool(s, "process_while_muted", false);
	obs_data_set_default_bool(s, "subtitle_save_srt", false);
	obs_data_set_default_bool(s, "truncate_output_file", false);
	obs_data_set_default_bool(s, "only_while_recording", false);
	obs_data_set_default_bool(s, "rename_file_to_match_recording", true);
	obs_data_set_default_int(s, "min_sub_duration", 1000);
	obs_data_set_default_int(s, "max_sub_duration", 3000);
	obs_data_set_default_bool(s, "advanced_settings", false);
	obs_data_set_default_double(s, "sentence_psum_accept_thresh", 0.4);
	obs_data_set_default_bool(s, "partial_group", true);
	obs_data_set_default_int(s, "partial_latency", 1100);

	// cloud translation options
	obs_data_set_default_bool(s, "translate_cloud", false);
	obs_data_set_default_string(s, "translate_cloud_provider", "google");
	obs_data_set_default_string(s, "translate_cloud_target_language", "en");
	obs_data_set_default_string(s, "translate_cloud_output", "none");
	obs_data_set_default_bool(s, "translate_cloud_only_full_sentences", true);
	obs_data_set_default_string(s, "translate_cloud_api_key", "");
	obs_data_set_default_string(s, "translate_cloud_secret_key", "");
	obs_data_set_default_bool(s, "translate_cloud_deepl_free", true);
	obs_data_set_default_string(s, "translate_cloud_region", "eastus");
	obs_data_set_default_string(s, "translate_cloud_endpoint",
				    "http://localhost:5000/translate");
	obs_data_set_default_string(
		s, "translate_cloud_body",
		"{\n\t\"text\":\"{{sentence}}\",\n\t\"target\":\"{{target_language}}\"\n}");
	obs_data_set_default_string(s, "translate_cloud_response_json_path", "translations.0.text");
}