/* @file
 * @brief Non-Volatile Storage (NVS) Read and Write a Value
 *
 * @section LICENCE
 *
 * This code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
*/

#include <type_traits>
#include <cstring>
#include <inttypes.h>
#include <string>
#include <nvs_flash.h>
#include <nvs.h>
#include <nvs_handle.hpp>
#include <esp_system.h>
#include <esp_log.h>

#include "nvs_device"
#include "nvstream"


namespace nvs
{

    //--[ Class device ]-----------------------------------------------------------------------------------------------

    // Static class for using as singleton
    // Representation of a nvs partition

    // Initialize default partition
    esp_err_t dev::Init()
    {
	ESP_LOGW(__func__, "Initialize the NVS device...");
	return nvs_flash_init();
    }; /* dev::Init */

    // Initialize partition with label 'partlabel'
    esp_err_t dev::Init(const std::string& partlabel)
    {
	ESP_LOGW(__func__, "Initialize the NVS device with label \"%s\"", partlabel.c_str());
	return nvs_flash_init_partition(partlabel.c_str());
    }; /* dev::Init */


    dev::dev()
    {
	// Initialize NVS
	ESP_LOGW(__func__, "Creating object of the NVS Device");
	err = dev::Init();
	ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    } /* device::device */

    dev::dev(const std::string& partlabel)
    {
	ESP_LOGW(__func__, "Creating object of the NVS Device with label: \"%s\"", partlabel.c_str());
	err = dev::Init(partlabel.c_str());
	ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    }; /* device::device */


    // Reinitialize partition manually
    esp_err_t dev::reInit()
    {
	ESP_LOGW(__func__, "Re-initialize the NVS device");
	err = dev::Init();
	ESP_ERROR_CHECK_WITHOUT_ABORT(err);
	return err;
    }; /* device::reInit */


    // get a device instance
    dev& dev::core()
    {
	    static dev instance;

	return instance;
    }; /* device::core */

    /// get the nvs device instance
    /// with sofisticated initialization:
    /// one pass reinit if first initialization
    dev& dev::partition()
    {
	ESP_LOGW(__func__, "Get the partition of the NVS device (singleton exemplar of object); test only");
	if (dev::state() == ESP_ERR_NVS_NO_FREE_PAGES || dev::state() == ESP_ERR_NVS_NEW_VERSION_FOUND)
	    core().reInit();
	return dev::core();
    };
    //	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    //	{
    //	    // NVS partition was truncated and needs to be erased
    //	    // Retry nvs_flash_init
    //	    ESP_ERROR_CHECK(nvs_flash_erase());
    //	    err = nvs_flash_init();
    //	}; /* if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) */
    //	ESP_ERROR_CHECK(err);


    // status of the device
    esp_err_t dev::status() {
	return err;
    }; /* device::status */


    // status of the device
    bool dev::isOK() {
	return (status() == ESP_OK)? true: false;
    }; /* device::isOK */

    // check the nvs partition status
    bool dev::check()
    {
	return dev::core().isOK();
    }; /* device::check */




    /// get handler from the handler storage uint32_t
    /// Inner procedure
    nvs_handle_t& handler(uint32_t& handler_stor) {
	return static_cast<nvs_handle_t&>(handler_stor); };

    /// obtain the uint32_t handler store from the handler
    /// Inner procedure
    uint32_t storhandler(nvs_handle_t handler) {
	return static_cast<uint32_t>(handler); };


    nvs_open_mode_t openmode2nvs(open_mode mode)
    {
	switch (mode)
	{
	case readwrite:
	    return NVS_READWRITE;
	case readonly:
	default:
	    return NVS_READONLY;
	}; /* switch mode */
    }; /* openmode2nvs() */

    ///--[ Class nvs::stream ]-----------------------------------------------------------------------------------------

    /// Manipulation with the NVS device namespaces


    /// Specialization of the stream::get_size() for the std::string type
    template <>
    size_t stream::get_size<std::string>(const std::string& name);

    /// Specialization of the stream::get_size() for the char[] type
    template <>
    size_t stream::get_size<char[]>(const std::string& name);

    /// Specialization of the stream::get_size() for the 'void' type (implied the 'blob' item)
    template <>
    size_t stream::get_size<void>(const std::string& name);

    /// Specialization of the stream::get_size() for the 'void*' type (implied the 'blob' item)
    template <>
    size_t stream::get_size<void*>(const std::string& name);



    stream::stream(): err(ESP_ERR_NVS_INVALID_STATE) {};


    stream::stream(const std::string& spacename, open_mode mode)
    {
	ESP_LOGI(__func__, "Create nvs::stream object with namespace name \"%s\"", spacename.c_str());
	open(spacename, mode);
    }; /* stream::stream */

    stream::~stream()
    {
	close();
    }; /* stream::~stream() */

    esp_err_t stream::open(const std::string& name, open_mode mode)
    {
	ESP_LOGI(__func__, "Open the nvs namespace with name \"%s\"", name.c_str());
	//dev::partition();
	if (dev::check())
	{
	    err = nvs_open(name.c_str(), openmode2nvs(mode), &handler(store));
	    ESP_LOGI(__func__, "Initializing NVS namespase is OK");
	}
	else
	{
	    err = ESP_ERR_NVS_INVALID_STATE;
	    ESP_LOGE(__func__, "Error initializing NVS namespase %s !!!", name.c_str());
	}; /* else if dev::check() */
	return err;
    }; /* stream::open */


    esp_err_t stream::close()
    {
	nvs_close(handler(store));
	store = 0;
	err = ESP_OK;
	return err;
    }; /* stream::close() */

    esp_err_t  stream::read_blob(const std::string& name, void* item, size_t& length)
    {
	err = (dev::core().isOK())? nvs_get_blob(handler(store), name.c_str(), item, &length): ESP_ERR_NVS_INVALID_STATE;
	return err;
    }; /* stream::read_blob() */

    esp_err_t stream::write_blob(const std::string& name, const void* item, size_t length)
    {
	err = (dev::core().isOK())? nvs_set_blob(handler(store), name.c_str(), item, length): ESP_ERR_NVS_INVALID_STATE;
	return err;
    }; /* stream::set_blob */

    esp_err_t stream::commit()
    {
	err = (dev::check())? nvs_commit(handler(store)): ESP_ERR_NVS_INVALID_STATE;
	if (err == ESP_OK)
	    clr_chngst();
	return err;
    }; /* stream::commit */

    /// set the changing state of the nvs::stream
    inline void stream::set_chgst()
    {
	chg_st = true;
    }; /* stream::set_chgst() */



/// Implemented using types:
///    integer types: uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t
///    zero-terminated string
///    variable length binary data (blob)

    template <typename T>
    struct type
    {
	static const char name[];
	static const char fmt[];
	static const size_t prnw;
    }; /* type */

    template <> const char type<int8_t  >::name[] = "int8_t";
    template <> const char type<uint8_t >::name[] = "uint8_t";
    template <> const char type<int16_t >::name[] = "int16_t";
    template <> const char type<uint16_t>::name[] = "uint16_t";
    template <> const char type<int32_t >::name[] = "int32_t";
    template <> const char type<uint32_t>::name[] = "uint32_t";
    template <> const char type<int64_t >::name[] = "int64_t";
    template <> const char type<uint64_t>::name[] = "uint64_t";

    template <> const char type<char*>::name[] = "char*";
    template <> const char type<std::string>::name[] = "std::string";
    template <> const char type<void> ::name[] = "void";
    template <> const char type<void*>::name[] = "void*";

    template <> const char type<int8_t>  ::fmt[] = "%0*" PRIi8;
    template <> const char type<uint8_t> ::fmt[] = "%0*" PRIu8;
    template <> const char type<int16_t> ::fmt[] = "%0*" PRIi16;
    template <> const char type<uint16_t>::fmt[] = "%0*" PRIu16;
    template <> const char type<int32_t> ::fmt[] = "%0*" PRIi32;
    template <> const char type<uint32_t>::fmt[] = "%0*" PRIu32;
    template <> const char type<int64_t> ::fmt[] = "%0*" PRIi64;
    template <> const char type<uint64_t>::fmt[] = "%0*" PRIu64;

    template <> const size_t type<uint8_t >::prnw  = 3;
    template <> const size_t type<int8_t  >::prnw  = type<uint8_t>::prnw + 1;
    template <> const size_t type<uint16_t>::prnw  = 5;
    template <> const size_t type<int16_t >::prnw  = type<uint16_t>::prnw + 1;
    template <> const size_t type<uint32_t>::prnw  = 10;
    template <> const size_t type<int32_t >::prnw  = type<uint32_t>::prnw + 1;
    template <> const size_t type<uint64_t>::prnw  = 20;
    template <> const size_t type<int64_t >::prnw  = type<uint64_t>::prnw + 1;



    /// @brief output values of any types value into C-string in a correct/compatible way, primarilly int-types.
    template <typename T>
    std::string printf_helper(T item);
    ///@brief buffer for string output of sended any int
    static char prnoutbuff[type<int64_t>::prnw + 1];

    template <typename T>
    std::string printf_helper(T item)
    {
	sprintf(prnoutbuff, type<T>::fmt, type<T>::prnw, item);
	return prnoutbuff;
    }; /* printf_helper() */



    /// @brief implementation of the read/write operation
    template <typename ItemT>
    class stream::core
    {
    public:
	/// @brief read core template procedure
	template <esp_err_t (*read_action)(nvs_handle_t, const char*, ItemT*)>
	static inline esp_err_t read(stream* nvstor, const std::string& name, ItemT &out);

	/// @brief write core template procedure
	template <esp_err_t (*write_action)(nvs_handle_t, const char*, ItemT)>
	static inline esp_err_t write(stream* nvstor, const std::string& name, ItemT item);
    }; /* template class nvs::stream::core */


    // Read the int32_t item from the NVS namespace
    /// @brief read core template procedure
    template <typename ItemT>
    template <esp_err_t (*read_action)(nvs_handle_t, const char*, ItemT*)>
    inline esp_err_t nvs::stream::core<ItemT>::read(stream* nvstream, const std::string& name, ItemT &out)
    {
	ESP_LOGW(__func__, "Read the %s item '%s', old value is: %s"/*"%'i"*/, type<ItemT>::name, name.c_str(), printf_helper(out).c_str());
	nvstream->err = read_action(handler(nvstream->store), name.c_str(), &out);
	ESP_LOGI(__func__, "                 New value of the %s is: %s"/*"%i"*/, name.c_str(), printf_helper(out).c_str());
	return nvstream->err;
    }; /* nvs::stream::core<ItemT>::write<read_action>() */




    /// @brief write core template procedure
    template <typename ItemT>
    template <esp_err_t (*write_action)(nvs_handle_t, const char*, ItemT item)>
    inline esp_err_t nvs::stream::core<ItemT>::write(stream* nvstream, const std::string& name, ItemT item)
    {
	    ItemT tmpval = 0;

// TODO --> temporarily changed from it: read(name, tmpval);
	nvstream->read(name, tmpval);
	ESP_LOGW(__PRETTY_FUNCTION__, "New item value \"%s\" value is: %s,\n\t\treaded item %s\n\t\terr state is: %i", name.c_str(), printf_helper(item).c_str(), printf_helper(tmpval).c_str(), nvstream->err);
	if ((nvstream->err == ESP_OK || nvstream->err == ESP_ERR_NVS_NOT_FOUND) && tmpval != item)
	{
	    ESP_LOGW(__func__, "Saving the new value of the Item %s", name.c_str());
	    nvstream->err = write_action(handler(nvstream->store), name.c_str(), item);

	    if (nvstream->err == ESP_OK)
		nvstream->set_chgst();
	}; /* 	if err == ESP_OK && tmpval != item */
	ESP_LOGW(__PRETTY_FUNCTION__, "Change state is: %s", nvstream->chg_st? "Yes": "No");
	return nvstream->err;
    }; /* nvs::stream::core<ItemT>::write<write_action>() */




    // Read the int8_t item from the NVS namespace
    template <>
    esp_err_t stream::read<int8_t>(const std::string& name, int8_t& item)
    {
	//err = nvs_get_i8(handle, name.c_str(), item);
	err = stream::core<int8_t>::read<nvs_get_i8>(this, name.c_str(), item);
	return err;
    }; /* stream::read<int8_t>() */
    template esp_err_t stream::read(const std::string&, int8_t&);


    // Read the 'char' from the NVS namespace
    template <>
    esp_err_t stream::read<char>(const std::string& name, char& item)
    {
	ESP_LOGW(__func__, "Read the char item '%s', old value is: %c", name.c_str(), item);
	read<int8_t>(name, reinterpret_cast<int8_t&>(item));
	ESP_LOGI(__func__, "               New value of the %s is: %c", name.c_str(), item);
	return err;
    }; /* stream::read<char>() */
    template esp_err_t stream::read<char>(const std::string&, char&);

    ///XXX operation 'read<char>()' was duplicated & redefined the 'read<int8_t>()', therefore is not needed
    ///XXX operation 'read<signed char>()' was duplicated & redefined the 'read<int8_t>()', therefore is not needed


    // Read the 'bool' from the NVS namespace
    template <>
    esp_err_t stream::read<bool>(const std::string& name, bool& item)
    {
	    char c = item? '1': '0';

	ESP_LOGW(__func__, "Read the bool item '%s', old value is: [%s]", name.c_str(), item? "True": "False");
	read<char>(name, c);
	item = !(c == '0');
	ESP_LOGI(__func__, "               New value of the %s is: [%s]", name.c_str(), item? "True": "False");
	return err;
    }; /* stream::read<char>() */
    template esp_err_t stream::read(const std::string&, bool&);


    // Read the int16_t item from the NVS namespace
    template <>
    esp_err_t stream::read<int16_t>(const std::string& name, int16_t& item)
    {
	//err = nvs_get_i16(handle, name.c_str(), item);
	err = stream::core<int16_t>::read<nvs_get_i16>(this, name.c_str(), item);
	return err;
    }; /* stream::read<int16_t>() */
    template esp_err_t stream::read(const std::string&, int16_t&);


    // Read the int32_t item from the NVS namespace
    template <>
    esp_err_t stream::read<int32_t>(const std::string& name, int32_t& item)
    {
	//err = nvs_get_i32(handle, name.c_str(), item);
	err = stream::core<int32_t>::read<nvs_get_i32>(this, name.c_str(), item);
	return err;
    }; /* stream::read<int32_t>() */
    template esp_err_t stream::read<int32_t>(const std::string&, int32_t&);


    // Read the int64_t item from the NVS namespace
    template <>
    esp_err_t stream::read<int64_t>(const std::string& name, int64_t& item)
    {
	//err = nvs_get_i64(handler(store), name.c_str(), item);
	err = stream::core<int64_t>::read<nvs_get_i64>(this, name.c_str(), item);
	return err;
    }; /* stream::read<int64_t>() */
    template esp_err_t stream::read(const std::string&, int64_t&);



    // Read the std::string item from the NVS namespace
    template <>
    esp_err_t stream::read<std::string>(const std::string& name, std::string &item)
    {
	ESP_LOGW(__func__, "Read the char[] item '%s', old value is: \"%s\"", name.c_str(), item.c_str());
	    size_t bufsz = get_size<std::string>(name);
	    char *buf = nullptr;

	/// Select maximum between size of the stored item and a length of 'item'
	if (err == ESP_OK)
	    bufsz = ((item.length() + 1) > bufsz)? (item.length() + 1): bufsz;
	else
	    return err;

	buf = new char[bufsz];
    	strcpy(buf, item.c_str());
    	err = nvs_get_str(handler(store), name.c_str(), buf, &bufsz);
    	item = buf;
    	delete[] buf;
	ESP_LOGI(__func__, "                 New value of the %s is: \"%s\", new buffer size is: %d", name.c_str(), item.c_str(), bufsz);
    	return err;
    }; /* stream::read<std::string>() */
    template esp_err_t stream::read(const std::string&, std::string&);



    // Read the uint8_t item from the NVS namespace
    template <>
    esp_err_t stream::read<uint8_t>(const std::string& name, uint8_t& item)
    {
	//err = nvs_get_u8(handle, name.c_str(), item);
	err = stream::core<uint8_t>::read<nvs_get_u8>(this, name.c_str(), item);
	return err;
    }; /* stream::read<uint8_t>() */
    template esp_err_t stream::read(const std::string&, uint8_t&);

    ///XXX operation 'read<unsigned char>()' was duplicated & redefined the 'read<uint8_t>()', therefore is not needed


    // Read the uint16_t item from the NVS namespace
    template <>
    esp_err_t stream::read<uint16_t>(const std::string& name, uint16_t& item)
    {
	//err = nvs_get_u16(handle, name.c_str(), item);
	err = stream::core<uint16_t>::read<nvs_get_u16>(this, name.c_str(), item);
	return err;
    }; /* stream::read<uint16_t>() */
    template esp_err_t stream::read(const std::string& name, uint16_t& item);


    // Read the uint32_t item from the NVS namespace
    template <>
    esp_err_t stream::read<uint32_t>(const std::string& name, uint32_t& item)
    {
	//err = nvs_get_i32(handle, name.c_str(), item);
	err = stream::core<uint32_t>::read<nvs_get_u32>(this, name.c_str(), item);
	return err;
    }; /* stream::read<uint32_t>() */
    template esp_err_t stream::read(const std::string&, uint32_t&);


    // Read the uint64_t item from the NVS namespace
    template <>
    esp_err_t stream::read<uint64_t>(const std::string& name, uint64_t& item)
    {
	//return nvs_get_i64(handle, name.c_str(), item);
	err = stream::core<uint64_t>::read<nvs_get_u64>(this, name.c_str(), item);
	return err;
    }; /* stream::read<uint64_t>() */
    template esp_err_t stream::read<uint64_t>(const std::string&, uint64_t&);




    ///@brief Write the int8_t item to the NVS namespace
    template <>
    esp_err_t stream::write<int8_t>(const std::string& name, int8_t item)
    {
	// err = nvs_set_i8(handle, name.c_str(), item);
	return (err = stream::core<int8_t>::write<nvs_set_i8>(this, name.c_str(), item));
	//return err;
    }; /* stream::write<int8_t>() */
    template esp_err_t stream::write(const std::string&, int8_t);

    ///XXX operation write<char>() and a write<bool>() was defined in the 'nvstream' file as inline
    ///XXX operation 'write<signed char>()' was duplicated & redefined the 'write<uint8_t>()', therefore is not needed

    ///@brief Write the int16_t item to the NVS namespace
    template <>
    esp_err_t stream::write<int16_t>(const std::string& name, int16_t item)
    {
	//err = nvs_set_i16(handle, name.c_str(), item);
	err = stream::core<int16_t>::write<nvs_set_i16>(this, name.c_str(), item);
	return err;
    }; /* stream::write<int16_t>() */
    template esp_err_t stream::write(const std::string&, int16_t);

    ///@brief Write the int32_t item to the NVS namespace
    template <>
    esp_err_t stream::write<int32_t>(const std::string& name, int32_t item)
    {
	//err = nvs_set_i32(handle, name.c_str(), item);
	err = nvs::stream::core<int32_t>::write<nvs_set_i32>(this, name.c_str(), item);
	return err;
    }; /* stream::write<int32_t>() */
    template esp_err_t stream::write(const std::string&, int32_t);

    ///@brief Write the int64_t item to the NVS namespace
    template <>
    esp_err_t stream::write<int64_t>(const std::string& name, int64_t item)
    {
	//err = nvs_set_i64(handle, name.c_str(), item);
	err = nvs::stream::core<int64_t>::write<nvs_set_i64>(this, name.c_str(), item);
	return err;
    }; /* stream::write<int64_t>() */
    template esp_err_t stream::write(const std::string&, int64_t);


    ///@brief Write the const char[] item to the NVS namespace
    template <>
    esp_err_t stream::write<const char[]>(const std::string& name, const char item[])
    {
	    size_t size = get_size<char[]>(name);

	if (err == ESP_OK && size == strlen(item))
	{
		std::string tmpstr = "";

	    read(name, tmpstr);
	    if (err == ESP_OK && tmpstr == item)
		return err;
	}; /* if err == ESP_OK && size == item.length() */
	ESP_LOGW(__func__, "Write the C-string char[] item '%s', value is: %s", name.c_str(), item);
	err = nvs_set_str(handler(store), name.c_str(), item);
	if (err == ESP_OK)
	    set_chgst();
	return err;
    }; /* stream::write<const char[]>() */
    template esp_err_t stream::write<const char[]>(const std::string& name, const char item[]);

    ///@brief Write the const std::string& item to the NVS namespace
    template <>
    esp_err_t stream::write<const std::string&>(const std::string& name, const std::string& item)
    {
	    size_t size = get_size<std::string>(name);

	if (err == ESP_OK && size == item.length())
	{
		std::string tmpstr = "";

	    read(name, tmpstr);
	    if (err == ESP_OK && tmpstr == item)
		return err;
	}; /* if err == ESP_OK && size == item.length() */

	ESP_LOGW(__func__, "Write the std::string item '%s', value is: %s", name.c_str(), item.c_str());
	err = nvs_set_str(handler(store), name.c_str(), item.c_str());
	if (err == ESP_OK)
	    set_chgst();
	return err;
    }; /* stream::write<const std::string&>() */
    template esp_err_t stream::write(const std::string&, const std::string&);



    ///@brief Write the uint8_t item to the NVS namespace
    template <>
    esp_err_t stream::write<uint8_t>(const std::string& name, uint8_t item)
    {
	//err = nvs_set_u8(handle, name.c_str(), item);
	err = nvs::stream::core<uint8_t>::write<nvs_set_u8>(this, name.c_str(), item);
	return err;
    }; /* stream::write<uint8_t>() */
    template esp_err_t stream::write(const std::string&, uint8_t);

    ///XXX operation 'write<unsigned char>()' was duplicated & redefined the 'write<uint8_t>()', therefore is not needed

    // Write the uint16_t item to the NVS namespace
    template <>
    esp_err_t stream::write<uint16_t>(const std::string& name, uint16_t item)
    {
	//err = nvs_set_u16(handle, name.c_str(), item);
	err = nvs::stream::core<uint16_t>::write<nvs_set_u16>(this, name.c_str(), item);
	return err;
    }; /* stream::write<uint16_t>() */
    template esp_err_t stream::write(const std::string&, uint16_t);

    // Write the uint32_t item to the NVS namespace
    template <>
    esp_err_t stream::write<uint32_t>(const std::string& name, uint32_t item)
    {
	//err = nvs_set_u32(handle, name.c_str(), item);
	err = nvs::stream::core<uint32_t>::write<nvs_set_u32>(this, name.c_str(), item);
	return err;
    }; /* stream::write<uint32_t>() */
    template esp_err_t stream::write(const std::string&, uint32_t);


    // Write the uint64_t item to the NVS namespace
    template <>
    esp_err_t stream::write<uint64_t>(const std::string& name, uint64_t item)
    {
	//err = nvs_set_u64(handle, name.c_str(), item);
	err = stream::core<uint64_t>::write<nvs_set_u64>(this, name.c_str(), item);
	return err;
    }; /* stream::write<uint64_t>() */
    template esp_err_t stream::write(const std::string&, uint64_t);


    ///@brief write c-string to nvs storage
    esp_err_t stream::write_str(const std::string& name, const char* item)
    {
	err = nvs_set_str(handler(store), name.c_str(), item);
	return err;
    }; /* stream::write_str() */

    ///@brief read c-string from nvs storage
    esp_err_t  stream::read_str(const std::string& name, char* item, size_t& length)
    {
	 err = nvs_get_str(handler(store), name.c_str(), item, &length);
	 return err;
    }; /* stream::read_str() */




    /**
     * size_t stream::get_size(const std::string& name);
     * get size of the item named 'name';
     * defined for the std::string, char* (length of stored string) or
     * void*, void (length of the blob)
     *
     * @param name: name of the item
     *
     * @return	item length: existing item with correct type
     * 			-1 : item non exist or with other types
     */

    /// Specialization of the stream::get_size() for the std::string type
    template <>
    size_t stream::get_size<std::string>(const std::string& name)
    {
	    size_t size = -1;

	err = nvs_get_str(handler(store), name.c_str(), NULL, &size);
	ESP_LOGW(__func__, "Get size of the %s with type <std::string>, size is: %i, returned error state is: %i", name.c_str(), size, err);
	return (err == ESP_OK)? size: -1;
    }; /* stream::get_size<std::string>() */
    template size_t stream::get_size<std::string>(const std::string& name);

    /// Specialization of the stream::get_size() for the char[] type
    template <>
    size_t stream::get_size<char[]>(const std::string& name) {
	ESP_LOGW(__func__, "Get size of the %s with type <char[]> (or a <char*>), redirected to a stream::get_size<std::string>()", name.c_str());
	return get_size<std::string>(name); }

    template size_t stream::get_size<char[]>(const std::string& name);

    /// Specialization of the stream::get_size() for the 'void' type (implied the 'blob' item)
    template <>
    size_t stream::get_size<void>(const std::string& name)
    {
	    size_t size = -1;	// TODO stub only!!! Modify it!!!

	err = nvs_get_blob(handler(store), name.c_str(), NULL, &size);
	ESP_LOGW(__func__, "Get size of the %s with type <void> (implied the 'blob' item), size is: %i, returned error state is: %i", name.c_str(), size, err);
	return (err == ESP_OK)? size: -1;
    }; /* stream::get_size<void>() */
    template size_t stream::get_size<void>(const std::string& name);

    /// Specialization of the stream::get_size() for the 'void*' type (implied the 'blob' item)
    template <>
    size_t stream::get_size<void*>(const std::string& name) {
	ESP_LOGW(__func__, "Get size of the %s with type <void*> (implied the 'blob' item), redirected to a stream::get_size<void>()", name.c_str());
	return get_size<void>(name); }
    template size_t stream::get_size<void*>(const std::string& name);



}; /* namespace nvs */

