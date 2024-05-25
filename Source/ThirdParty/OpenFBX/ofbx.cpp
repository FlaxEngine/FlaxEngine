#include "ofbx.h"
#include "libdeflate.h"
#include <cassert>
#include <math.h>
#include <ctype.h>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <inttypes.h>
#include <string.h>

#if __cplusplus >= 202002L
#include <bit> // for std::bit_cast (C++20 and later)
#endif
#include <map>

namespace ofbx
{

static int decodeIndex(int idx)
{
	return (idx < 0) ? (-idx - 1) : idx;
}

static int codeIndex(int idx, bool last)
{
	return last ? (-idx - 1) : idx;
}

template <typename T>
static T& emplace_back(std::vector<T>& vec) {
	vec.emplace_back();
	return vec.back();
}

struct Allocator {
	struct Page {
		struct {
			Page* next = nullptr;
			u32 offset = 0;
		} header;
		u8 data[4096 * 1024 - 12];
	};
	Page* first = nullptr;

	~Allocator() {
		Page* p = first;
		while (p) {
			Page* n = p->header.next;
			delete p;
			p = n;
		}
	}

	template <typename T, typename... Args> T* allocate(Args&&... args)
	{
		assert(sizeof(T) <= sizeof(first->data));
		if (!first) {
			first = new Page;
		}
		Page* p = first;
		if (p->header.offset % alignof(T) != 0) {
			p->header.offset += alignof(T) - p->header.offset % alignof(T);
		}

		if (p->header.offset + sizeof(T) > sizeof(p->data)) {
			p = new Page;
			p->header.next = first;
			first = p;
		}
		T* res = new (p->data + p->header.offset) T(args...);
		p->header.offset += sizeof(T);
		return res;
	}
};


struct Video
{
	IElementProperty* base64_property = nullptr;
	DataView filename;
	DataView content;
	DataView media;
	bool is_base_64;
};


struct Error
{
	Error() {}
	Error(const char* msg)
	{
		s_message = msg;
	}

	// Format a message with printf-style arguments.
	template <typename... Args>
	Error(const char* fmt, Args... args) 
	{
		char buf[1024];
		std::snprintf(buf, sizeof(buf), fmt, args...);
		s_message = buf;
	}

	static const char* s_message;
};


const char* Error::s_message = "";


template <typename T> struct OptionalError
{
	OptionalError(Error error)
		: is_error(true)
	{
	}


	OptionalError(T _value)
		: value(_value)
		, is_error(false)
	{
	}


	T getValue() const
	{
#ifdef _DEBUG
		assert(error_checked);
#endif
		return value;
	}


	bool isError()
	{
#ifdef _DEBUG
		error_checked = true;
#endif
		return is_error;
	}


private:
	T value;
	bool is_error;
#ifdef _DEBUG
	bool error_checked = false;
#endif
};


#pragma pack(1)
struct Header
{
	u8 magic[21];
	u8 reserved[2];
	u32 version;
};
#pragma pack()


struct Cursor
{
	const u8* current;
	const u8* begin;
	const u8* end;
};


static void setTranslation(const DVec3& t, DMatrix* mtx)
{
	mtx->m[12] = t.x;
	mtx->m[13] = t.y;
	mtx->m[14] = t.z;
}


static DVec3 operator-(const DVec3& v)
{
	return {-v.x, -v.y, -v.z};
}


static DMatrix operator*(const DMatrix& lhs, const DMatrix& rhs)
{
	DMatrix res;
	for (int j = 0; j < 4; ++j)
	{
		for (int i = 0; i < 4; ++i)
		{
			double tmp = 0;
			for (int k = 0; k < 4; ++k)
			{
				tmp += lhs.m[i + k * 4] * rhs.m[k + j * 4];
			}
			res.m[i + j * 4] = tmp;
		}
	}
	return res;
}


static DMatrix makeIdentity()
{
	return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}


static DMatrix rotationX(double angle)
{
	DMatrix m = makeIdentity();
	double c = cos(angle);
	double s = sin(angle);

	m.m[5] = m.m[10] = c;
	m.m[9] = -s;
	m.m[6] = s;

	return m;
}


static DMatrix rotationY(double angle)
{
	DMatrix m = makeIdentity();
	double c = cos(angle);
	double s = sin(angle);

	m.m[0] = m.m[10] = c;
	m.m[8] = s;
	m.m[2] = -s;

	return m;
}


static DMatrix rotationZ(double angle)
{
	DMatrix m = makeIdentity();
	double c = cos(angle);
	double s = sin(angle);

	m.m[0] = m.m[5] = c;
	m.m[4] = -s;
	m.m[1] = s;

	return m;
}


static DMatrix getRotationMatrix(const DVec3& euler, RotationOrder order)
{
	const double TO_RAD = 3.1415926535897932384626433832795028 / 180.0;
	DMatrix rx = rotationX(euler.x * TO_RAD);
	DMatrix ry = rotationY(euler.y * TO_RAD);
	DMatrix rz = rotationZ(euler.z * TO_RAD);
	switch (order)
	{
		default:
		case RotationOrder::EULER_XYZ: return rz * ry * rx;
		case RotationOrder::EULER_XZY: return ry * rz * rx;
		case RotationOrder::EULER_YXZ: return rz * rx * ry;
		case RotationOrder::EULER_YZX: return rx * rz * ry;
		case RotationOrder::EULER_ZXY: return ry * rx * rz;
		case RotationOrder::EULER_ZYX: return rx * ry * rz;
		case RotationOrder::SPHERIC_XYZ: assert(false); Error::s_message = "Unsupported rotation order."; return rx * ry * rz;
	}
}


double fbxTimeToSeconds(i64 value)
{
	return double(value) / 46186158000L;
}


i64 secondsToFbxTime(double value)
{
	return i64(value * 46186158000L);
}


static DVec3 operator*(const DVec3& v, float f)
{
	return {v.x * f, v.y * f, v.z * f};
}


static DVec3 operator+(const DVec3& a, const DVec3& b)
{
	return {a.x + b.x, a.y + b.y, a.z + b.z};
}

static FVec3 operator+(const FVec3& a, const FVec3& b)
{
	return {a.x + b.x, a.y + b.y, a.z + b.z};
}


template <int SIZE> static bool copyString(char (&destination)[SIZE], const char* source)
{
	const char* src = source;
	char* dest = destination;
	int length = SIZE;
	if (!src) return false;

	while (*src && length > 1)
	{
		*dest = *src;
		--length;
		++dest;
		++src;
	}
	*dest = 0;
	return *src == '\0';
}


u64 DataView::toU64() const
{
	if (is_binary)
	{
		assert(end - begin == sizeof(u64));
		u64 result;
		memcpy(&result, begin, sizeof(u64));
		return result;
	}
	static_assert(sizeof(unsigned long long) >= sizeof(u64), "can't use strtoull");
	return strtoull((const char*)begin, nullptr, 10);
}


i64 DataView::toI64() const
{
	if (is_binary)
	{
		assert(end - begin == sizeof(i64));
		i64 result;
		memcpy(&result, begin, sizeof(i64));
		return result;
	}
	static_assert(sizeof(long long) >= sizeof(i64), "can't use atoll");
	return atoll((const char*)begin);
}


int DataView::toInt() const
{
	if (is_binary)
	{
		assert(end - begin == sizeof(int));
		int result;
		memcpy(&result, begin, sizeof(int));
		return result;
	}
	return atoi((const char*)begin);
}


u32 DataView::toU32() const
{
	if (is_binary)
	{
		assert(end - begin == sizeof(u32));
		u32 result;
		memcpy(&result, begin, sizeof(u32));
		return result;
	}
	return (u32)atoll((const char*)begin);
}

bool DataView::toBool() const
{
	return toInt() != 0;
}


double DataView::toDouble() const
{
	if (is_binary)
	{
		assert(end - begin == sizeof(double));
		double result;
		memcpy(&result, begin, sizeof(double));
		return result;
	}
	return atof((const char*)begin);
}


float DataView::toFloat() const
{
	if (is_binary)
	{
		assert(end - begin == sizeof(float));
		float result;
		memcpy(&result, begin, sizeof(float));
		return result;
	}
	return (float)atof((const char*)begin);
}


bool DataView::operator==(const char* rhs) const
{
	if (!begin) return !rhs[0];
	const char* c = rhs;
	const char* c2 = (const char*)begin;
	while (*c && c2 != (const char*)end)
	{
		if (*c != *c2) return false;
		++c;
		++c2;
	}
	return *c2 == '\0' || c2 == (const char*)end && *c == '\0';
}


struct Property;
struct Element;

template <typename T> static bool parseMemory(const Property& property, T* out, int max_size_bytes);
template <typename T> static bool parseVecData(Property& property, std::vector<T>* out_vec);
template <typename T> static bool parseVertexData(const Element& element, const char* name, const char* index_name, T& out, std::vector<struct ParseDataJob>& jobs);
static bool parseDouble(Property& property, double* out);

struct ParseDataJob {
	using F = bool (*)(Property*, void*);
	Property* property = nullptr;
	void* data = nullptr;
	bool error = false;
	F f;
};

template <typename T> [[nodiscard]] bool pushJob(std::vector<ParseDataJob>& jobs, Property& prop, std::vector<T>& data) {
	ParseDataJob& job = emplace_back(jobs);
	job.property = &prop;
	job.data = (void*)&data;
	job.f = [](Property* prop, void* data){ return parseVecData(*prop, (std::vector<T>*)data); };
	return true;
}

struct Property : IElementProperty
{
	Type getType() const override { return (Type)type; }
	IElementProperty* getNext() const override { return next; }
	DataView getValue() const override { return value; }
	int getCount() const override
	{
		assert(type == ARRAY_DOUBLE || type == ARRAY_INT || type == ARRAY_FLOAT || type == ARRAY_LONG);
		if (value.is_binary)
		{
			int i;
			memcpy(&i, value.begin, sizeof(i));
			return i;
		}
		return count;
	}

	bool getValues(double* values, int max_size) const override { return parseMemory(*this, values, max_size); }

	bool getValues(float* values, int max_size) const override { return parseMemory(*this, values, max_size); }

	bool getValues(u64* values, int max_size) const override { return parseMemory(*this, values, max_size); }

	bool getValues(i64* values, int max_size) const override { return parseMemory(*this, values, max_size); }

	bool getValues(int* values, int max_size) const override { return parseMemory(*this, values, max_size); }

	int count = 0;
	u8 type = INTEGER;
	DataView value;
	Property* next = nullptr;
};

struct Element : IElement
{
	IElement* getFirstChild() const override { return child; }
	IElement* getSibling() const override { return sibling; }
	DataView getID() const override { return id; }
	IElementProperty* getFirstProperty() const override { return first_property; }
	IElementProperty* getProperty(int idx) const
	{
		IElementProperty* prop = first_property;
		for (int i = 0; i < idx; ++i)
		{
			if (prop == nullptr) return nullptr;
			prop = prop->getNext();
		}
		return prop;
	}

	DataView id;
	Element* child = nullptr;
	Element* sibling = nullptr;
	Property* first_property = nullptr;
};


static const Element* findChild(const Element& element, const char* id)
{
	Element* const* iter = &element.child;
	while (*iter)
	{
		if ((*iter)->id == id) return *iter;
		iter = &(*iter)->sibling;
	}
	return nullptr;
}


static IElement* resolveProperty(const Object& obj, const char* name, bool* is_p60)
{
	*is_p60 = false;
	const Element* props = findChild((const Element&)obj.element, "Properties70");
	if (!props) {
		props = findChild((const Element&)obj.element, "Properties60");
		*is_p60 = true;
		if (!props) return nullptr;
	}

	Element* prop = props->child;
	while (prop)
	{
		if (prop->first_property && prop->first_property->value == name)
		{
			return prop;
		}
		prop = prop->sibling;
	}
	return nullptr;
}


static int resolveEnumProperty(const Object& object, const char* name, int default_value)
{
	bool is_p60;
	Element* element = (Element*)resolveProperty(object, name, &is_p60);
	if (!element) return default_value;
	Property* x = (Property*)element->getProperty(is_p60 ? 3 : 4);
	if (!x) return default_value;

	return x->value.toInt();
}


static DVec3 resolveVec3Property(const Object& object, const char* name, const DVec3& default_value)
{
	bool is_p60;
	Element* element = (Element*)resolveProperty(object, name, &is_p60);
	if (!element) return default_value;
	Property* x = (Property*)element->getProperty(is_p60 ? 3 : 4);
	if (!x || !x->next || !x->next->next) return default_value;

	return {x->value.toDouble(), x->next->value.toDouble(), x->next->next->value.toDouble()};
}

static bool isString(const Property* prop)
{
	if (!prop) return false;
	return prop->getType() == Property::STRING;
}


static bool isLong(const Property* prop)
{
	if (!prop) return false;
	return prop->getType() == Property::LONG;
}

static bool decompress(const u8* in, size_t in_size, u8* out, size_t out_size)
{
	auto dec = libdeflate_alloc_decompressor();
	size_t dummy;
	bool res = libdeflate_deflate_decompress(dec, in + 2, in_size - 2, out, out_size, &dummy) == LIBDEFLATE_SUCCESS;
	libdeflate_free_decompressor(dec);
	return res;
}


template <typename T> static OptionalError<T> read(Cursor* cursor)
{
	if (cursor->current + sizeof(T) > cursor->end) return Error("Reading past the end");
	T value = *(const T*)cursor->current;
	cursor->current += sizeof(T);
	return value;
}


static OptionalError<DataView> readShortString(Cursor* cursor)
{
	DataView value;
	OptionalError<u8> length = read<u8>(cursor);
	if (length.isError()) return Error();

	if (cursor->current + length.getValue() > cursor->end) return Error("Reading past the end");
	value.begin = cursor->current;
	cursor->current += length.getValue();

	value.end = cursor->current;

	return value;
}


static OptionalError<DataView> readLongString(Cursor* cursor)
{
	DataView value;
	OptionalError<u32> length = read<u32>(cursor);
	if (length.isError()) return Error();

	if (cursor->current + length.getValue() > cursor->end) return Error("Reading past the end");
	value.begin = cursor->current;
	cursor->current += length.getValue();

	value.end = cursor->current;

	return value;
}

// 	Cheat sheet: //
/*
'S': Long string
'Y': 16-bit signed integer
'C': 8-bit signed integer
'I': 32-bit signed integer
'F': Single precision floating-point number
'D': Double precision floating-point number
'L': 64-bit signed integer
'R': Binary data
'b', 'f', 'd', 'l', 'c' and 'i': Arrays of binary data

Src: https://code.blender.org/2013/08/fbx-binary-file-format-specification/
*/

static OptionalError<Property*> readProperty(Cursor* cursor, Allocator& allocator)
{
	if (cursor->current == cursor->end) return Error("Reading past the end");

	Property* prop = allocator.allocate<Property>();
	prop->next = nullptr;
	prop->type = *cursor->current;
	++cursor->current;
	prop->value.begin = cursor->current;

	switch (prop->type)
	{
		case 'S':
		{
			OptionalError<DataView> val = readLongString(cursor);
			if (val.isError()) return Error();
			prop->value = val.getValue();
			break;
		}
		case 'Y': cursor->current += 2; break;
		case 'C': cursor->current += 1; break;
		case 'I': cursor->current += 4; break;
		case 'F': cursor->current += 4; break;
		case 'D': cursor->current += 8; break;
		case 'L': cursor->current += 8; break;
		case 'R':
		{
			OptionalError<u32> len = read<u32>(cursor);
			if (len.isError()) return Error();
			if (cursor->current + len.getValue() > cursor->end) return Error("Reading past the end");
			cursor->current += len.getValue();
			break;
		}
		case 'b':
		case 'c':
		case 'f':
		case 'd':
		case 'l':
		case 'i':
		{
			OptionalError<u32> length = read<u32>(cursor);
			OptionalError<u32> encoding = read<u32>(cursor);
			OptionalError<u32> comp_len = read<u32>(cursor);
			if (length.isError() || encoding.isError() || comp_len.isError()) return Error();
			if (cursor->current + comp_len.getValue() > cursor->end) return Error("Reading past the end");
			cursor->current += comp_len.getValue();
			break;
		}
		default:
		{
			char str[32];
			snprintf(str, sizeof(str), "Unknown property type: %c", prop->type);
			return Error(str);
		}
	}
	prop->value.end = cursor->current;
	return prop;
}

static OptionalError<u64> readElementOffset(Cursor* cursor, u32 version)
{
	if (version >= 7500)
	{
		OptionalError<u64> tmp = read<u64>(cursor);
		if (tmp.isError()) return Error();
		return tmp.getValue();
	}

	OptionalError<u32> tmp = read<u32>(cursor);
	if (tmp.isError()) return Error();
	return tmp.getValue();
}


static OptionalError<Element*> readElement(Cursor* cursor, u32 version, Allocator& allocator)
{
	OptionalError<u64> end_offset = readElementOffset(cursor, version);
	if (end_offset.isError()) return Error();
	if (end_offset.getValue() == 0) return nullptr;

	OptionalError<u64> prop_count = readElementOffset(cursor, version);
	OptionalError<u64> prop_length = readElementOffset(cursor, version);
	if (prop_count.isError() || prop_length.isError()) return Error();

	OptionalError<DataView> id = readShortString(cursor);
	if (id.isError()) return Error();

	Element* element = allocator.allocate<Element>();
	element->first_property = nullptr;
	element->id = id.getValue();

	element->child = nullptr;
	element->sibling = nullptr;

	Property** prop_link = &element->first_property;
	for (u32 i = 0; i < prop_count.getValue(); ++i)
	{
		OptionalError<Property*> prop = readProperty(cursor, allocator);
		if (prop.isError())
		{
			return Error();
		}

		*prop_link = prop.getValue();
		prop_link = &(*prop_link)->next;
	}

	if (cursor->current - cursor->begin >= (ptrdiff_t)end_offset.getValue()) return element;

	int BLOCK_SENTINEL_LENGTH = version >= 7500 ? 25 : 13;

	Element** link = &element->child;
	while (cursor->current - cursor->begin < ((ptrdiff_t)end_offset.getValue() - BLOCK_SENTINEL_LENGTH))
	{
		OptionalError<Element*> child = readElement(cursor, version, allocator);
		if (child.isError())
		{
			return Error();
		}

		*link = child.getValue();
		if (child.getValue() == 0) break;
		link = &(*link)->sibling;
	}

	if (cursor->current + BLOCK_SENTINEL_LENGTH > cursor->end)
	{
		return Error("Reading past the end");
	}

	cursor->current += BLOCK_SENTINEL_LENGTH;
	return element;
}


static bool isEndLine(const Cursor& cursor)
{
	return *cursor.current == '\n' || *cursor.current == '\r' && cursor.current + 1 < cursor.end && *(cursor.current + 1) != '\n';
}


static void skipInsignificantWhitespaces(Cursor* cursor)
{
	while (cursor->current < cursor->end && isspace(*cursor->current) && !isEndLine(*cursor))
	{
		++cursor->current;
	}
}


static void skipLine(Cursor* cursor)
{
	while (cursor->current < cursor->end && !isEndLine(*cursor))
	{
		++cursor->current;
	}
	if (cursor->current < cursor->end) ++cursor->current;
	skipInsignificantWhitespaces(cursor);
}


static void skipWhitespaces(Cursor* cursor)
{
	while (cursor->current < cursor->end && isspace(*cursor->current))
	{
		++cursor->current;
	}
	while (cursor->current < cursor->end && *cursor->current == ';') skipLine(cursor);
}


static bool isTextTokenChar(char c)
{
	return isalnum(c) || c == '_' || c == '-';
}


static DataView readTextToken(Cursor* cursor)
{
	DataView ret;
	ret.begin = cursor->current;
	while (cursor->current < cursor->end && isTextTokenChar(*cursor->current))
	{
		++cursor->current;
	}
	ret.end = cursor->current;
	return ret;
}


static OptionalError<Property*> readTextProperty(Cursor* cursor, Allocator& allocator)
{
	Property* prop = allocator.allocate<Property>();
	prop->value.is_binary = false;
	prop->next = nullptr;
	if (*cursor->current == '"')
	{
		prop->type = 'S';
		++cursor->current;
		prop->value.begin = cursor->current;
		while (cursor->current < cursor->end && *cursor->current != '"')
		{
			++cursor->current;
		}
		prop->value.end = cursor->current;
		if (cursor->current < cursor->end) ++cursor->current; // skip '"'
		return prop;
	}

	if (isdigit(*cursor->current) || *cursor->current == '-')
	{
		prop->type = 'L';
		prop->value.begin = cursor->current;
		if (*cursor->current == '-') ++cursor->current;
		while (cursor->current < cursor->end && isdigit(*cursor->current))
		{
			++cursor->current;
		}
		prop->value.end = cursor->current;

		if (cursor->current < cursor->end && *cursor->current == '.')
		{
			prop->type = 'D';
			++cursor->current;
			while (cursor->current < cursor->end && isdigit(*cursor->current))
			{
				++cursor->current;
			}
			if (cursor->current < cursor->end && (*cursor->current == 'e' || *cursor->current == 'E'))
			{
				// 10.5e-013
				++cursor->current;
				if (cursor->current < cursor->end && *cursor->current == '-') ++cursor->current;
				while (cursor->current < cursor->end && isdigit(*cursor->current)) ++cursor->current;
			}


			prop->value.end = cursor->current;
		}
		else if (cursor->current < cursor->end && (*cursor->current == 'e' || *cursor->current == 'E')) {
			prop->type = 'D';
			// 10e-013
			++cursor->current;
			if (cursor->current < cursor->end && *cursor->current == '-') ++cursor->current;
			while (cursor->current < cursor->end && isdigit(*cursor->current)) ++cursor->current;
			prop->value.end = cursor->current;
		}
		return prop;
	}

	if (*cursor->current == 'T' || *cursor->current == 'Y' || *cursor->current == 'W' || *cursor->current == 'C')
	{
		// WTF is this
		prop->type = *cursor->current;
		prop->value.begin = cursor->current;
		++cursor->current;
		prop->value.end = cursor->current;
		return prop;
	}

	if (*cursor->current == ',') {
		// https://github.com/nem0/OpenFBX/issues/85
		prop->type = IElementProperty::NONE;
		prop->value.begin = cursor->current;
		prop->value.end = cursor->current;
		return prop;
	}

	if (*cursor->current == '*')
	{
		prop->type = 'l';
		++cursor->current;
		// Vertices: *10740 { a: 14.2760353088379,... }
		while (cursor->current < cursor->end && *cursor->current != ':')
		{
			++cursor->current;
		}
		if (cursor->current < cursor->end) ++cursor->current; // skip ':'
		skipInsignificantWhitespaces(cursor);
		prop->value.begin = cursor->current;
		prop->count = 0;
		bool is_any = false;
		while (cursor->current < cursor->end && *cursor->current != '}')
		{
			if (*cursor->current == ',')
			{
				if (is_any) ++prop->count;
				is_any = false;
			}
			else if (!isspace(*cursor->current) && !isEndLine(*cursor))
				is_any = true;
			if (*cursor->current == '.') prop->type = 'd';
			++cursor->current;
		}
		if (is_any) ++prop->count;
		prop->value.end = cursor->current;
		if (cursor->current < cursor->end) ++cursor->current; // skip '}'
		return prop;
	}

	assert(false);
	return Error("Unknown error");
}


static OptionalError<Element*> readTextElement(Cursor* cursor, Allocator& allocator)
{
	DataView id = readTextToken(cursor);
	if (cursor->current == cursor->end) return Error("Unexpected end of file");
	if (*cursor->current != ':') return Error("Unexpected character");
	++cursor->current;

	skipInsignificantWhitespaces(cursor);
	if (cursor->current == cursor->end) return Error("Unexpected end of file");

	Element* element = allocator.allocate<Element>();
	element->id = id;

	Property** prop_link = &element->first_property;
	while (cursor->current < cursor->end && !isEndLine(*cursor) && *cursor->current != '{')
	{
		OptionalError<Property*> prop = readTextProperty(cursor, allocator);
		if (prop.isError())
		{
			return Error();
		}
		if (cursor->current < cursor->end && *cursor->current == ',')
		{
			++cursor->current;
			skipWhitespaces(cursor);
		}
		skipInsignificantWhitespaces(cursor);

		*prop_link = prop.getValue();
		prop_link = &(*prop_link)->next;
	}

	Element** link = &element->child;
	if (*cursor->current == '{')
	{
		++cursor->current;
		skipWhitespaces(cursor);
		while (cursor->current < cursor->end && *cursor->current != '}')
		{
			OptionalError<Element*> child = readTextElement(cursor, allocator);
			if (child.isError())
			{
				return Error();
			}
			skipWhitespaces(cursor);

			*link = child.getValue();
			link = &(*link)->sibling;
		}
		if (cursor->current < cursor->end) ++cursor->current; // skip '}'
	}
	return element;
}


static OptionalError<Element*> tokenizeText(const u8* data, size_t size, Allocator& allocator)
{
	Cursor cursor;
	cursor.begin = data;
	cursor.current = data;
	cursor.end = data + size;

	Element* root = allocator.allocate<Element>();
	root->first_property = nullptr;
	root->id.begin = nullptr;
	root->id.end = nullptr;
	root->child = nullptr;
	root->sibling = nullptr;

	Element** element = &root->child;
	while (cursor.current < cursor.end)
	{
		if (*cursor.current == ';' || *cursor.current == '\r' || *cursor.current == '\n')
		{
			skipLine(&cursor);
		}
		else
		{
			OptionalError<Element*> child = readTextElement(&cursor, allocator);
			if (child.isError())
			{
				return Error();
			}
			*element = child.getValue();
			if (!*element) return root;
			element = &(*element)->sibling;
		}
	}

	return root;
}


static OptionalError<Element*> tokenize(const u8* data, size_t size, u32& version, Allocator& allocator) {
	if (size < sizeof(Header)) return Error("Invalid header");

	Cursor cursor;
	cursor.begin = data;
	cursor.current = data;
	cursor.end = data + size;

#if __cplusplus >= 202002L
	const Header* header = std::bit_cast<const Header*>(cursor.current);
#else
	Header header_temp;
	memcpy(&header_temp, cursor.current, sizeof(Header));
	const Header* header = &header_temp;
#endif

	cursor.current += sizeof(Header);
	version = header->version;

	Element* root = allocator.allocate<Element>();
	root->first_property = nullptr;
	root->id.begin = nullptr;
	root->id.end = nullptr;
	root->child = nullptr;
	root->sibling = nullptr;

	Element** element = &root->child;
	for (;;)
	{
		OptionalError<Element*> child = readElement(&cursor, header->version, allocator);
		if (child.isError())
		{
			return Error();
		}

		*element = child.getValue();
		if (!*element) return root;
		element = &(*element)->sibling;
	}
}

static void parseTemplates(const Element& root)
{
	const Element* defs = findChild(root, "Definitions");
	if (!defs) return;

	std::unordered_map<std::string, Element*> templates;
	Element* def = defs->child;
	while (def)
	{
		if (def->id == "ObjectType")
		{
			Element* subdef = def->child;
			while (subdef)
			{
				if (subdef->id == "PropertyTemplate")
				{
					DataView prop1 = def->first_property->value;
					DataView prop2 = subdef->first_property->value;
					std::string key((const char*)prop1.begin, prop1.end - prop1.begin);
					key += std::string((const char*)prop1.begin, prop1.end - prop1.begin);
					templates[key] = subdef;
				}
				subdef = subdef->sibling;
			}
		}
		def = def->sibling;
	}
	// TODO
}


struct Scene;

enum class VertexDataMapping {
	BY_POLYGON_VERTEX,
	BY_POLYGON,
	BY_VERTEX
};

struct Vec2AttributesImpl {
	std::vector<Vec2> values;
	std::vector<int> indices;
	VertexDataMapping mapping;
	operator Vec2Attributes() const {
		return { values.data(), indices.data(), int(indices.empty() ? values.size() : indices.size()) };
	}
};

struct Vec3AttributesImpl {
	std::vector<Vec3> values;
	std::vector<int> indices;
	VertexDataMapping mapping;
	operator Vec3Attributes() const {
		return { values.data(), indices.data(), int(indices.empty() ? values.size() : indices.size()), int(values.size()) };
	}
};

struct Vec4AttributesImpl {
	std::vector<Vec4> values;
	std::vector<int> indices;
	VertexDataMapping mapping;
	operator Vec4Attributes() const {
		return { values.data(), indices.data(), int(indices.empty() ? values.size() : indices.size()) };
	}
};

struct GeometryPartitionImpl {
	std::vector<GeometryPartition::Polygon> polygons;
	int max_polygon_triangles = 0;
	int triangles_count = 0;
};

struct GeometryDataImpl : GeometryData {
	Vec3AttributesImpl positions;
	Vec3AttributesImpl normals;
	Vec3AttributesImpl tangents;
	Vec4AttributesImpl colors;
	Vec2AttributesImpl uvs[Geometry::s_uvs_max];
	std::vector<GeometryPartitionImpl> partitions;
	
	std::vector<int> materials;

	template <typename T, typename S>
	T patchAttributes(const S& attr) const {
		T res = attr;
		if (!attr.values.empty() && attr.mapping == VertexDataMapping::BY_VERTEX && attr.indices.empty()) {
			res.indices = positions.indices.data();
			res.count = int(positions.indices.size());
		}
		return res;
	}

	Vec3Attributes getPositions() const override { return positions; }
	Vec3Attributes getNormals() const override { return patchAttributes<Vec3Attributes>(normals); }
	Vec2Attributes getUVs(int index) const override { return patchAttributes<Vec2Attributes>(uvs[index]); }
	Vec4Attributes getColors() const override { return patchAttributes<Vec4Attributes>(colors); }
	Vec3Attributes getTangents() const override { return patchAttributes<Vec3Attributes>(tangents); }
	int getPartitionCount() const override { return (int)partitions.size(); }
	
	GeometryPartition getPartition(int index) const override { 
		if (index >= partitions.size()) return {nullptr, 0, 0, 0};
		return {
			partitions[index].polygons.data(),
			int(partitions[index].polygons.size()),
			partitions[index].max_polygon_triangles,
			partitions[index].triangles_count
		};
	}

	template <typename T>
	bool postprocess(T& attr) {
		if (attr.values.empty()) return true;
		if (attr.mapping == VertexDataMapping::BY_VERTEX && !attr.indices.empty()) {
			if (positions.indices.empty()) return false; // not supported

			std::vector<int> remapped;
			attr.mapping = VertexDataMapping::BY_POLYGON_VERTEX;
			remapped.resize(positions.indices.size());
			for (int i = 0; i < remapped.size(); ++i) {
				remapped[i] = attr.indices[decodeIndex(positions.indices[i])];
			}
			attr.indices = remapped;
		}
		else if (attr.mapping == VertexDataMapping::BY_POLYGON) {
			if (!attr.indices.empty()) return false; // not supported
			if (partitions.size() != 1) return false; // not supported
			if (partitions[0].polygons.size() != attr.values.size()) return false; // invalid

			std::vector<int> remapped;
			attr.mapping = VertexDataMapping::BY_POLYGON_VERTEX;
			remapped.resize(positions.indices.size());

			for (int i = 0, c = (int)partitions[0].polygons.size(); i < c; ++i) {
				GeometryPartition::Polygon& polygon = partitions[0].polygons[i];
				for (int j = polygon.from_vertex; j < polygon.from_vertex + polygon.vertex_count; ++j) {
					remapped[j] = i;
				}
			}
			attr.indices = remapped;
		}
		return true;
	}

	bool postprocess() {
		if (materials.empty()) {
			GeometryPartitionImpl& partition = emplace_back(partitions);
			int polygon_count = 0;
			for (int i : positions.indices) {
				if (i < 0) ++polygon_count;
			}
			partition.polygons.reserve(polygon_count);
			int polygon_start = 0;
			int max_polygon_triangles = 0;
			int total_triangles = 0;
			int* indices = positions.indices.data();
			for (int i = 0, c = (int)positions.indices.size(); i < c; ++i) {
				if (indices[i] < 0) {
					int vertex_count = i - polygon_start + 1;
					if (vertex_count > 2) {
						partition.polygons.push_back({polygon_start, vertex_count});
						indices[i] = -indices[i] - 1;
						int triangles = vertex_count - 2;
						total_triangles += triangles;
						if (triangles > max_polygon_triangles) max_polygon_triangles = triangles;
					}
					polygon_start = i + 1;
				}
			}
			partition.max_polygon_triangles = max_polygon_triangles;
			partition.triangles_count = total_triangles;
		}
		else {
			int max_partition = 0;
			for (int m : materials) {
				if (m > max_partition) max_partition = m;
			}
			partitions.resize(max_partition + 1);

			u32 polygon_idx = 0;
			int* indices = positions.indices.data();
			int num_polygon_vertices = 0;
			int polygon_start = 0;
			for (int i = 0, c = (int)positions.indices.size(); i < c; ++i) {
				++num_polygon_vertices;
				if (indices[i] < 0) {
					u32 material_index = materials[polygon_idx];
					GeometryPartitionImpl& partition = partitions[material_index];
					partition.polygons.push_back({polygon_start, num_polygon_vertices});

					int triangles = num_polygon_vertices - 2;
					partition.triangles_count += triangles;
					if (triangles > partition.max_polygon_triangles) partition.max_polygon_triangles = triangles;
					
					indices[i] = -indices[i] - 1;

					polygon_start = i + 1;
					++polygon_idx;
					num_polygon_vertices = 0;
				}
			}
		}

		postprocess(normals);
		postprocess(tangents);
		for (Vec2AttributesImpl& uv : uvs) postprocess(uv);
		postprocess(colors);

		return true;
	}
};


Mesh::Mesh(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}

struct GeometryImpl : Geometry, GeometryDataImpl {
	const Skin* skin = nullptr;
	const BlendShape* blendShape = nullptr;

	GeometryImpl(const Scene& _scene, const IElement& _element)
		: Geometry(_scene, _element)
	{
	}

	Type getType() const override { return Type::GEOMETRY; }
	const GeometryData& getGeometryData() const override { return *this; }
	const Skin* getSkin() const override { return skin; }
	const BlendShape* getBlendShape() const override { return blendShape; }
};

struct MeshImpl : Mesh
{
	MeshImpl(const Scene& _scene, const IElement& _element)
		: Mesh(_scene, _element)
	{
		is_node = true;
	}


	DMatrix getGeometricMatrix() const override
	{
		DVec3 translation = resolveVec3Property(*this, "GeometricTranslation", {0, 0, 0});
		DVec3 rotation = resolveVec3Property(*this, "GeometricRotation", {0, 0, 0});
		DVec3 scale = resolveVec3Property(*this, "GeometricScaling", {1, 1, 1});

		DMatrix scale_mtx = makeIdentity();
		scale_mtx.m[0] = (float)scale.x;
		scale_mtx.m[5] = (float)scale.y;
		scale_mtx.m[10] = (float)scale.z;
		DMatrix mtx = getRotationMatrix(rotation, RotationOrder::EULER_XYZ);
		setTranslation(translation, &mtx);

		return scale_mtx * mtx;
	}

	Type getType() const override { return Type::MESH; }

	const Pose* getPose() const override { return pose; }
	const Geometry* getGeometry() const override { return geometry; }
	const Material* getMaterial(int index) const override { return materials[index]; }
	int getMaterialCount() const override { return (int)materials.size(); }

	const GeometryData& getGeometryData() const override { return geometry ? static_cast<const GeometryData&>(*geometry) : geometry_data; }
	const Skin* getSkin() const override { return geometry ? geometry->getSkin() : skin; }
	const BlendShape* getBlendShape() const override { return geometry ? geometry->getBlendShape() : blendShape; }

	const Pose* pose = nullptr;
	const GeometryImpl* geometry = nullptr;
	std::vector<const Material*> materials;
	const Skin* skin = nullptr;
	const BlendShape* blendShape = nullptr;

	// old formats do not use Geometry nodes but embed vertex data directly in Mesh
	GeometryDataImpl geometry_data;
};


Material::Material(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


struct MaterialImpl : Material
{
	MaterialImpl(const Scene& _scene, const IElement& _element)
		: Material(_scene, _element)
	{
		for (const Texture*& tex : textures) tex = nullptr;
	}

	Type getType() const override { return Type::MATERIAL; }

	const Texture* getTexture(Texture::TextureType type) const override { return textures[type]; }
	Color getDiffuseColor() const override { return diffuse_color; }
	Color getSpecularColor() const override { return specular_color; }
	Color getReflectionColor() const override { return reflection_color; };
	Color getAmbientColor() const override { return ambient_color; };
	Color getEmissiveColor() const override { return emissive_color; };

	double getDiffuseFactor() const override { return diffuse_factor; };
	double getSpecularFactor() const override { return specular_factor; };
	double getReflectionFactor() const override { return reflection_factor; };
	double getShininess() const override { return shininess; };
	double getShininessExponent() const override { return shininess_exponent; };
	double getAmbientFactor() const override { return ambient_factor; };
	double getBumpFactor() const override { return bump_factor; };
	double getEmissiveFactor() const override { return emissive_factor; };

	const Texture* textures[Texture::TextureType::COUNT];
	Color diffuse_color;
	Color specular_color;
	Color reflection_color;
	Color ambient_color;
	Color emissive_color;

	double diffuse_factor;
	double specular_factor;
	double reflection_factor;
	double shininess;
	double shininess_exponent;
	double ambient_factor;
	double bump_factor;
	double emissive_factor;
 };


struct LimbNodeImpl : Object
{
	LimbNodeImpl(const Scene& _scene, const IElement& _element)
		: Object(_scene, _element)
	{
		is_node = true;
	}
	Type getType() const override { return Type::LIMB_NODE; }
};


struct NullImpl : Object
{
	NullImpl(const Scene& _scene, const IElement& _element)
		: Object(_scene, _element)
	{
		is_node = true;
	}
	Type getType() const override { return Type::NULL_NODE; }
};


NodeAttribute::NodeAttribute(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


struct NodeAttributeImpl : NodeAttribute
{
	NodeAttributeImpl(const Scene& _scene, const IElement& _element)
		: NodeAttribute(_scene, _element)
	{
	}
	Type getType() const override { return Type::NODE_ATTRIBUTE; }
	DataView getAttributeType() const override { return attribute_type; }


	DataView attribute_type;
};


Geometry::Geometry(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


Shape::Shape(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


struct ShapeImpl : Shape {
	std::vector<Vec3> vertices;
	std::vector<Vec3> normals;
	std::vector<int> indices;

	ShapeImpl(const Scene& _scene, const IElement& _element)
		: Shape(_scene, _element)
	{}

	bool postprocess(GeometryImpl& geom, Allocator& allocator);

	Type getType() const override { return Type::SHAPE; }
	int getVertexCount() const override { return (int)vertices.size(); }
	int getIndexCount() const override { return (int)indices.size(); }
	const Vec3* getVertices() const override { return &vertices[0]; }
	const Vec3* getNormals() const override { return normals.empty() ? nullptr : &normals[0]; }
	const int* getIndices() const override { return indices.empty() ? nullptr : &indices[0]; }
};


Cluster::Cluster(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


struct ClusterImpl : Cluster
{
	ClusterImpl(const Scene& _scene, const IElement& _element)
		: Cluster(_scene, _element)
	{
	}

	const int* getIndices() const override { return &indices[0]; }
	int getIndicesCount() const override { return (int)indices.size(); }
	const double* getWeights() const override { return &weights[0]; }
	int getWeightsCount() const override { return (int)weights.size(); }
	DMatrix getTransformMatrix() const override { return transform_matrix; }
	DMatrix getTransformLinkMatrix() const override { return transform_link_matrix; }
	Object* getLink() const override { return link; }

	bool postprocess() {
		assert(skin);

		GeometryDataImpl* geom = static_cast<GeometryDataImpl*>(static_cast<GeometryImpl*>(skin->resolveObjectLinkReverse(Object::Type::GEOMETRY)));
		if (!geom) {
			MeshImpl* mesh = (MeshImpl*)skin->resolveObjectLinkReverse(Object::Type::MESH);
			if(!mesh) return false;
			geom = &mesh->geometry_data;
		}

		const Element* indexes = findChild((const Element&)element, "Indexes");
		if (indexes && indexes->first_property)
		{
			if (!parseVecData(*indexes->first_property, &indices)) return false;
		}

		const Element* weights_el = findChild((const Element&)element, "Weights");
		if (weights_el && weights_el->first_property)
		{
			if (!parseVecData(*weights_el->first_property, &weights)) return false;
		}

		return true;
	}


	Object* link = nullptr;
	Skin* skin = nullptr;
	std::vector<int> indices;
	std::vector<double> weights;
	DMatrix transform_matrix;
	DMatrix transform_link_matrix;
	Type getType() const override { return Type::CLUSTER; }
};


AnimationStack::AnimationStack(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


AnimationLayer::AnimationLayer(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


AnimationCurve::AnimationCurve(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


AnimationCurveNode::AnimationCurveNode(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


struct AnimationStackImpl : AnimationStack
{
	AnimationStackImpl(const Scene& _scene, const IElement& _element)
		: AnimationStack(_scene, _element)
	{
	}


	const AnimationLayer* getLayer(int index) const override
	{
		return resolveObjectLink<AnimationLayer>(index);
	}


	Type getType() const override { return Type::ANIMATION_STACK; }
};


struct AnimationCurveImpl : AnimationCurve
{
	AnimationCurveImpl(const Scene& _scene, const IElement& _element)
		: AnimationCurve(_scene, _element)
	{
	}

	int getKeyCount() const override { return (int)times.size(); }
	const i64* getKeyTime() const override { return &times[0]; }
	const float* getKeyValue() const override { return &values[0]; }

	std::vector<i64> times;
	std::vector<float> values;
	Type getType() const override { return Type::ANIMATION_CURVE; }
};


Skin::Skin(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


struct SkinImpl : Skin
{
	SkinImpl(const Scene& _scene, const IElement& _element)
		: Skin(_scene, _element)
	{
	}

	int getClusterCount() const override { return (int)clusters.size(); }
	const Cluster* getCluster(int idx) const override { return clusters[idx]; }

	Type getType() const override { return Type::SKIN; }

	std::vector<Cluster*> clusters;
};


BlendShapeChannel::BlendShapeChannel(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


struct BlendShapeChannelImpl : BlendShapeChannel
{
	BlendShapeChannelImpl(const Scene& _scene, const IElement& _element)
		: BlendShapeChannel(_scene, _element)
	{
	}

	double getDeformPercent() const override { return deformPercent; }
	int getShapeCount() const override { return (int)shapes.size(); }
	const Shape* getShape(int idx) const override { return shapes[idx]; }

	Type getType() const override { return Type::BLEND_SHAPE_CHANNEL; }

	bool postprocess(Allocator& allocator) {
		assert(blendShape);

		GeometryImpl* geom = (GeometryImpl*)blendShape->resolveObjectLinkReverse(Object::Type::GEOMETRY);
		if (!geom) return false;

		const Element* deform_percent_el = findChild((const Element&)element, "DeformPercent");
		if (deform_percent_el && deform_percent_el->first_property)
		{
			if (!parseDouble(*deform_percent_el->first_property, &deformPercent)) return false;
		}

		const Element* full_weights_el = findChild((const Element&)element, "FullWeights");
		if (full_weights_el && full_weights_el->first_property)
		{
			if (!parseVecData(*full_weights_el->first_property, &fullWeights)) return false;
		}

		for (int i = 0; i < (int)shapes.size(); i++)
		{
			auto shape = (ShapeImpl*)shapes[i];
			if (!shape->postprocess(*geom, allocator)) return false;
		}

		return true;
	}


	BlendShape* blendShape = nullptr;
	double deformPercent = 0;
	std::vector<double> fullWeights;
	std::vector<Shape*> shapes;
};


BlendShape::BlendShape(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


struct BlendShapeImpl : BlendShape
{
	BlendShapeImpl(const Scene& _scene, const IElement& _element)
		: BlendShape(_scene, _element)
	{
	}

	int getBlendShapeChannelCount() const override { return (int)blendShapeChannels.size(); }
	const BlendShapeChannel* getBlendShapeChannel(int idx) const override { return blendShapeChannels[idx]; }

	Type getType() const override { return Type::BLEND_SHAPE; }

	std::vector<BlendShapeChannel*> blendShapeChannels;
};


Texture::Texture(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


Pose::Pose(const Scene& _scene, const IElement& _element)
	: Object(_scene, _element)
{
}


struct PoseImpl : Pose
{
	PoseImpl(const Scene& _scene, const IElement& _element)
		: Pose(_scene, _element)
	{}

	bool postprocess(Scene& scene);
	DMatrix getMatrix() const override { return matrix; }
	const Object* getNode() const override { return node; }
	Type getType() const override { return Type::POSE; }

	DMatrix matrix;
	Object* node = nullptr;
	u64 node_id;
};


struct TextureImpl : Texture
{
	TextureImpl(const Scene& _scene, const IElement& _element)
		: Texture(_scene, _element)
	{
	}

	DataView getRelativeFileName() const override { return relative_filename; }
	DataView getFileName() const override { return filename; }
	DataView getEmbeddedData() const override;

	DataView media;
	DataView filename;
	DataView relative_filename;
	Type getType() const override { return Type::TEXTURE; }
};

struct LightImpl : Light
{
	LightImpl(const Scene& _scene, const IElement& _element)
		: Light(_scene, _element)
	{
	}

	Type getType() const override { return Type::LIGHT; }
	LightType getLightType() const override { return lightType; }

	bool doesCastLight() const override { return castLight; }

	bool doesDrawVolumetricLight() const override
	{
		// Return the draw volumetric light property based on the stored data (WIP)
		return false;
	}

	bool doesDrawGroundProjection() const override
	{
		// Return the draw ground projection property based on the stored data (WIP)
		return false;
	}

	bool doesDrawFrontFacingVolumetricLight() const override
	{
		// Return the draw front-facing volumetric light property based on the stored data (WIP)
		return false;
	}

	Color getColor() const override { return color; }
	double getIntensity() const override { return intensity; }
	double getInnerAngle() const override { return innerAngle; }
	double getOuterAngle() const override { return outerAngle; }

	double getFog() const override { return fog; }

	DecayType getDecayType() const override { return decayType; }
	double getDecayStart() const override { return decayStart; }

	// Near attenuation
	bool doesEnableNearAttenuation() const override { return enableNearAttenuation; }
	double getNearAttenuationStart() const override { return nearAttenuationStart; }
	double getNearAttenuationEnd() const override { return nearAttenuationEnd; }

	// Far attenuation
	bool doesEnableFarAttenuation() const override { return enableFarAttenuation; }
	double getFarAttenuationStart() const override { return farAttenuationStart; }
	double getFarAttenuationEnd() const override { return farAttenuationEnd; }

	// Shadows
	const Texture* getShadowTexture() const override { return shadowTexture; }
	bool doesCastShadows() const override { return castShadows; }
	Color getShadowColor() const override { return shadowColor; }

	// Member variables to store light properties
	//-------------------------------------------------------------------------
	LightType lightType = LightType::POINT;
	bool castLight = true;
	Color color = {1, 1, 1};					// Light color (RGB values)
	double intensity = 100.0;

	double innerAngle = 0.0;
	double outerAngle = 45.0;

	double fog = 50;

	DecayType decayType = DecayType::QUADRATIC;
	double decayStart = 1.0;

	bool enableNearAttenuation = false;
	double nearAttenuationStart = 0.0;
	double nearAttenuationEnd = 0.0;

	bool enableFarAttenuation = false;
	double farAttenuationStart = 0.0;
	double farAttenuationEnd = 0.0;

	const Texture* shadowTexture = nullptr;
	bool castShadows = true;
	Color shadowColor = {0, 0, 0};
};

static float OFBX_PI = 3.14159265358979323846f;
struct CameraImpl : public Camera
{
	CameraImpl(const Scene& _scene, const IElement& _element)
		: Camera(_scene, _element)
	{
	}

	ProjectionType projectionType = ProjectionType::PERSPECTIVE;
	ApertureMode apertureMode = ApertureMode::HORIZONTAL; // Used to determine the FOV

	double filmHeight = 36.0;
	double filmWidth = 24.0;

	double aspectHeight = 1.0;
	double aspectWidth = 1.0;

	double nearPlane = 0.1;
	double farPlane = 1000.0;
	bool autoComputeClipPanes = true;
	
	GateFit gateFit = GateFit::HORIZONTAL;
	double filmAspectRatio = 1.0;
	double focalLength = 50.0;
	double focusDistance = 50.0;
	
	DVec3 backgroundColor = {0, 0, 0};
	DVec3 interestPosition = {0, 0, 0};

	double fieldOfView = 60.0;

	Type getType() const override { return Type::CAMERA; }
	ProjectionType getProjectionType() const override { return projectionType; }
	ApertureMode getApertureMode() const override { return apertureMode; }

	double getFilmHeight() const override { return filmHeight; }
	double getFilmWidth() const override { return filmWidth; }

	double getAspectHeight() const override { return aspectHeight; }
	double getAspectWidth() const override { return aspectWidth; }

	double getNearPlane() const override { return nearPlane; }
	double getFarPlane() const override { return farPlane; }
	bool doesAutoComputeClipPanes() const override { return autoComputeClipPanes; }

	GateFit getGateFit() const override { return gateFit; }
	double getFilmAspectRatio() const override { return filmAspectRatio; }
	double getFocalLength() const override { return focalLength; }
	double getFocusDistance() const override { return focusDistance; }

	DVec3 getBackgroundColor() const override { return backgroundColor; }
	DVec3 getInterestPosition() const override { return interestPosition; }

	void CalculateFOV()
	{
		switch (apertureMode)
		{
			case Camera::ApertureMode::HORIZONTAL:
				fieldOfView =  2.0 * atan(filmWidth / (2.0 * focalLength)) * 180.0 / OFBX_PI;
				return;
			case Camera::ApertureMode::VERTICAL:
				fieldOfView =  2.0 * atan(filmHeight / (2.0 * focalLength)) * 180.0 / OFBX_PI;
				return;
			case Camera::ApertureMode::HORIZANDVERT:
				fieldOfView =  2.0 * atan(sqrt(filmWidth * filmWidth + filmHeight * filmHeight) / (2.0 * focalLength)) * 180.0 / OFBX_PI;
				return;
			case Camera::ApertureMode::FOCALLENGTH:
				fieldOfView =  2.0 * atan(filmHeight / (2.0 * focalLength)) * 180.0 / OFBX_PI; // Same as vertical ¯\_(ツ)_/¯
				return;
			default:
				fieldOfView =  60.0;
		}
	}
};

struct Root : Object
{
	Root(const Scene& _scene, const IElement& _element)
		: Object(_scene, _element)
	{
		copyString(name, "RootNode");
		is_node = true;
	}
	Type getType() const override { return Type::ROOT; }
};


struct Scene : IScene
{
	struct Connection
	{
		enum Type
		{
			OBJECT_OBJECT,
			OBJECT_PROPERTY,
			PROPERTY_OBJECT,
			PROPERTY_PROPERTY,
		};

		Type type = OBJECT_OBJECT;
		u64 from_object = 0;
		u64 to_object = 0;
		DataView from_property;
		DataView to_property;
	};

	struct ObjectPair
	{
		const Element* element;
		Object* object;
	};


	int getAnimationStackCount() const override { return (int)m_animation_stacks.size(); }
	int getGeometryCount() const override { return (int)m_geometries.size(); }
	int getMeshCount() const override { return (int)m_meshes.size(); }
	float getSceneFrameRate() const override { return m_scene_frame_rate; }
	const GlobalSettings* getGlobalSettings() const override { return &m_settings; }

	const Object* const* getAllObjects() const override { return m_all_objects.empty() ? nullptr : &m_all_objects[0]; }


	int getAllObjectCount() const override { return (int)m_all_objects.size(); }

	int getEmbeddedDataCount() const override {
		return (int)m_videos.size();
	}

	DataView getEmbeddedData(int index) const override {
		return m_videos[index].content;
	}

	bool isEmbeddedBase64(int index) const override {
		return m_videos[index].is_base_64;
	}

	const IElementProperty* getEmbeddedBase64Data(int index) const override {
		return m_videos[index].base64_property;
	}

	DataView getEmbeddedFilename(int index) const override {
		return m_videos[index].filename;
	}

	const AnimationStack* getAnimationStack(int index) const override
	{
		assert(index >= 0);
		assert(index < m_animation_stacks.size());
		return m_animation_stacks[index];
	}


	const Mesh* getMesh(int index) const override
	{
		assert(index >= 0);
		assert(index < m_meshes.size());
		return m_meshes[index];
	}


	const Geometry* getGeometry(int index) const override
	{
		assert(index >= 0);
		assert(index < m_geometries.size());
		return m_geometries[index];
	}


	const TakeInfo* getTakeInfo(const char* name) const override
	{
		for (const TakeInfo& info : m_take_infos)
		{
			if (info.name == name) return &info;
		}
		return nullptr;
	}

	const Camera* getCamera(int index) const override
	{
		assert(index >= 0);
		assert(index < m_cameras.size());
		return m_cameras[index];
	}

	int getCameraCount() const override
	{
		return (int)m_cameras.size();
	}

	const Light* getLight(int index) const override
	{
		assert(index >= 0);
		assert(index < m_lights.size());
		return m_lights[index];
	}

	int getLightCount() const override
	{
		return (int)m_lights.size();
	}


	const IElement* getRootElement() const override { return m_root_element; }
	const Object* getRoot() const override { return m_root; }


	void destroy() override { delete this; }


	~Scene() override {
		for(Object* ptr : m_all_objects) {
			ptr->~Object();
		}
	}

	bool finalize();

	Element* m_root_element = nullptr;
	Root* m_root = nullptr;
	float m_scene_frame_rate = -1;
	GlobalSettings m_settings;

	std::unordered_map<std::string, u64> m_fake_ids;
	std::unordered_map<u64, ObjectPair> m_object_map;
	std::vector<Object*> m_all_objects;
	std::vector<Mesh*> m_meshes;
	std::vector<Geometry*> m_geometries;
	std::vector<AnimationStack*> m_animation_stacks;
	std::vector<Camera*> m_cameras;
	std::vector<Light*> m_lights;
	std::vector<Connection> m_connections;
	std::vector<u8> m_data;
	std::vector<TakeInfo> m_take_infos;
	std::vector<Video> m_videos;
	Allocator m_allocator;
	u32 version = 0;
};

Object::Object(const Scene& scene, const IElement& element)
	: scene(scene)
	, element(element)
	, is_node(false)
	, node_attribute(nullptr)
{
	Element& e = (Element&)element;
	if (scene.version < 6200 && e.first_property && isString(e.first_property)) {
		e.first_property->value.toString(name);
	}
	else if (e.first_property && e.first_property->next)
	{
		e.first_property->next->value.toString(name);
	}
	else
	{
		name[0] = '\0';
	}
}

DataView TextureImpl::getEmbeddedData() const {
	if (!media.begin) return media;
	for (const Video& v : scene.m_videos) {
		if (v.media.end - v.media.begin != media.end - media.begin) continue;
		const size_t len = v.media.end - v.media.begin;
		if (memcmp(v.media.begin, media.begin, len) != 0) continue;

		return v.content;
	}
	return {};
}


bool PoseImpl::postprocess(Scene& scene) {
	node = scene.m_object_map[node_id].object;
	if (node && node->getType() == Object::Type::MESH) {
		static_cast<MeshImpl*>(node)->pose = this;
	}
	return true;
}


struct AnimationCurveNodeImpl : AnimationCurveNode
{
	AnimationCurveNodeImpl(const Scene& _scene, const IElement& _element)
		: AnimationCurveNode(_scene, _element)
	{
		default_values[0] = default_values[1] = default_values[2] =  0;
		bool is_p60;
		Element* dx = static_cast<Element*>(resolveProperty(*this, "d|X", &is_p60));
		Element* dy = static_cast<Element*>(resolveProperty(*this, "d|Y", &is_p60));
		Element* dz = static_cast<Element*>(resolveProperty(*this, "d|Z", &is_p60));

		if (dx) {
			Property* x = (Property*)dx->getProperty(4);
			if (x) default_values[0] = (float)x->value.toDouble();
		}
		if (dy) {
			Property* y = (Property*)dy->getProperty(4);
			if (y) default_values[1] = (float)y->value.toDouble();
		}
		if (dz) {
			Property* z = (Property*)dz->getProperty(4);
			if (z) default_values[2] = (float)z->value.toDouble();
		}
	}


	const Object* getBone() const override
	{
		return bone;
	}

	DataView getBoneLinkProperty() const override { return bone_link_property; }

	const AnimationCurve* getCurve(int idx) const override {
		assert(idx >= 0 && idx < 3);
		return curves[idx].curve;
	}


	DVec3 getNodeLocalTransform(double time) const override
	{
		i64 fbx_time = secondsToFbxTime(time);

		auto getCoord = [&](const Curve& curve, i64 fbx_time, int idx) {
			if (!curve.curve) return default_values[idx];

			const i64* times = curve.curve->getKeyTime();
			const float* values = curve.curve->getKeyValue();
			int count = curve.curve->getKeyCount();

			if (fbx_time < times[0]) fbx_time = times[0];
			if (fbx_time > times[count - 1]) fbx_time = times[count - 1];
			for (int i = 1; i < count; ++i)
			{
				if (times[i] >= fbx_time)
				{
					float t = float(double(fbx_time - times[i - 1]) / double(times[i] - times[i - 1]));
					return values[i - 1] * (1 - t) + values[i] * t;
				}
			}
			return values[0];
		};

		return {getCoord(curves[0], fbx_time, 0), getCoord(curves[1], fbx_time, 1), getCoord(curves[2], fbx_time, 2)};
	}


	struct Curve
	{
		const AnimationCurve* curve = nullptr;
		const Scene::Connection* connection = nullptr;
	};


	Curve curves[3];
	Object* bone = nullptr;
	DataView bone_link_property;
	Type getType() const override { return Type::ANIMATION_CURVE_NODE; }
	float default_values[3];
	enum Mode
	{
		TRANSLATION,
		ROTATION,
		SCALE
	} mode = TRANSLATION;
};


struct AnimationLayerImpl : AnimationLayer
{
	AnimationLayerImpl(const Scene& _scene, const IElement& _element)
		: AnimationLayer(_scene, _element)
	{
	}


	Type getType() const override { return Type::ANIMATION_LAYER; }


	const AnimationCurveNode* getCurveNode(int index) const override
	{
		if (index >= (int)curve_nodes.size() || index < 0) return nullptr;
		return curve_nodes[index];
	}


	const AnimationCurveNode* getCurveNode(const Object& bone, const char* prop) const override
	{
		for (const AnimationCurveNodeImpl* node : curve_nodes)
		{
			if (node->bone_link_property.begin && node->bone_link_property == prop && node->bone == &bone) return node;
		}
		return nullptr;
	}


	std::vector<AnimationCurveNodeImpl*> curve_nodes;
};

/*
	DEBUGGING ONLY (but im not your boss so do what you want)
	- maps the contents of the given node for viewing in the debugger
	
	std::map<std::string, ofbx::IElementProperty*, std::less<>> allProperties;
	mapProperties(element, allProperties);
*/
void mapProperties(const ofbx::IElement& parent, std::map<std::string, ofbx::IElementProperty*, std::less<std::string>>& propMap)
{
	for (const ofbx::IElement* element = parent.getFirstChild(); element; element = element->getSibling())
	{
		char key[32];

		if (element->getFirstProperty())
			element->getFirstProperty()->getValue().toString(key);
		else
			element->getID().toString(key);


		ofbx::IElementProperty* prop = element->getFirstProperty();
		propMap.insert({key, prop});

		if (element->getFirstChild()) mapProperties(*element, propMap);
	}
};


void parseVideo(Scene& scene, const Element& element, Allocator& allocator)
{
	if (!element.first_property) return;
	if (!element.first_property->next) return;
	if (element.first_property->next->getType() != IElementProperty::STRING) return;

	const Element* content_element = findChild(element, "Content");

	bool is_base64 = false;
	if (!content_element) return;
	if (!content_element->first_property) return;
	const Property* content_property = content_element->first_property;
	if (content_element->first_property->getType() != IElementProperty::BINARY) {
		is_base64 = true;
	}

	const Element* filename_element = findChild(element, "Filename");
	if (!filename_element) return;
	if (!filename_element->first_property) return;
	if (filename_element->first_property->getType() != IElementProperty::STRING) return;

	Video video;
	video.is_base_64 = is_base64;
	video.base64_property = is_base64 ? content_element->first_property->next : nullptr;
	video.content = is_base64 ? DataView{} : content_element->first_property->value;
	video.filename = filename_element->first_property->value;
	video.media = element.first_property->next->value;
	scene.m_videos.push_back(video);
}

static bool parseGeometryMaterials(GeometryDataImpl& geom, const Element& element, std::vector<ParseDataJob> &jobs)
{
	const Element* layer_material_element = findChild(element, "LayerElementMaterial");
	if (!layer_material_element) return true;

	const Element* mapping_element = findChild(*layer_material_element, "MappingInformationType");
	const Element* reference_element = findChild(*layer_material_element, "ReferenceInformationType");

	if (!mapping_element || !reference_element) return false;
	if (!mapping_element->first_property) return false;
	if (!reference_element->first_property) return false;

	if (mapping_element->first_property->value == "ByPolygon" && reference_element->first_property->value == "IndexToDirect") {
		const Element* indices_element = findChild(*layer_material_element, "Materials");
		if (!indices_element || !indices_element->first_property) return false;

		return pushJob(jobs, *indices_element->first_property, geom.materials);
	}
	else
	{
		if (mapping_element->first_property->value != "AllSame") return false;
	}
	return true;
}

static bool parseGeometryUVs(GeometryDataImpl& geom, const Element& element, std::vector<ParseDataJob> &jobs) {
	const Element* layer_uv_element = findChild(element, "LayerElementUV");
	while (layer_uv_element) {
		const int uv_index = layer_uv_element->first_property ? layer_uv_element->first_property->getValue().toInt() : 0;
		if (uv_index >= 0 && uv_index < Geometry::s_uvs_max) {
			Vec2AttributesImpl& uvs = geom.uvs[uv_index];
			if (!parseVertexData(*layer_uv_element, "UV", "UVIndex", uvs, jobs)) return false;
		}

		do {
			layer_uv_element = layer_uv_element->sibling;
		} while (layer_uv_element && layer_uv_element->id != "LayerElementUV");
	}
	return true;
}

static bool parseGeometryTangents(GeometryDataImpl& geom, const Element& element, std::vector<ParseDataJob> &jobs) {
	const Element* layer_tangent_element = findChild(element, "LayerElementTangents");
	if (!layer_tangent_element) layer_tangent_element = findChild(element, "LayerElementTangent");
	if (!layer_tangent_element) return true;

	if (findChild(*layer_tangent_element, "Tangents")) {
		return parseVertexData(*layer_tangent_element, "Tangents", "TangentsIndex", geom.tangents, jobs);
	}

	return parseVertexData(*layer_tangent_element, "Tangent", "TangentIndex", geom.tangents, jobs);
}

static bool parseGeometryColors(GeometryDataImpl& geom, const Element& element, std::vector<ParseDataJob> &jobs) {
	const Element* layer_color_element = findChild(element, "LayerElementColor");
	if (!layer_color_element) return true;
	return parseVertexData(*layer_color_element, "Colors", "ColorIndex", geom.colors, jobs);
}

static bool parseGeometryNormals(GeometryDataImpl& geom, const Element& element, std::vector<ParseDataJob> &jobs) {
	const Element* layer_normal_element = findChild(element, "LayerElementNormal");
	if (!layer_normal_element) return true;
	return parseVertexData(*layer_normal_element, "Normals", "NormalsIndex", geom.normals, jobs);
}

struct OptionalError<Object*> parseMesh(const Scene& scene, const Element& element, std::vector<ParseDataJob> &jobs, Allocator& allocator) {
	MeshImpl* mesh = allocator.allocate<MeshImpl>(scene, element);

	if (!element.first_property) return Error("Invalid mesh");

	const Element* vertices_element = findChild(element, "Vertices");
	if (!vertices_element || !vertices_element->first_property) return mesh;

	const Element* polys_element = findChild(element, "PolygonVertexIndex");
	if (!polys_element || !polys_element->first_property) return Error("Indices missing");

	if (!pushJob(jobs, *vertices_element->first_property, mesh->geometry_data.positions.values)) return Error("Invalid vertices");
	if (!pushJob(jobs, *polys_element->first_property, mesh->geometry_data.positions.indices)) return Error("Invalid vertices");

	if (!parseGeometryMaterials(mesh->geometry_data, element, jobs)) return Error("Invalid materials");
	if (!parseGeometryUVs(mesh->geometry_data, element, jobs)) return Error("Invalid vertex attributes");
	if (!parseGeometryTangents(mesh->geometry_data, element, jobs)) return Error("Invalid vertex attributes");
	if (!parseGeometryColors(mesh->geometry_data, element, jobs)) return Error("Invalid vertex attributes");
	if (!parseGeometryNormals(mesh->geometry_data, element, jobs)) return Error("Invalid vertex attributes");

	return mesh;
}

struct OptionalError<Object*> parseTexture(const Scene& scene, const Element& element, Allocator& allocator)
{
	TextureImpl* texture = allocator.allocate<TextureImpl>(scene, element);
	const Element* texture_filename = findChild(element, "FileName");
	if (texture_filename && texture_filename->first_property)
	{
		texture->filename = texture_filename->first_property->value;
	}

	const Element* media = findChild(element, "Media");
	if (media && media->first_property)
	{
		texture->media = media->first_property->value;
	}

	const Element* texture_relative_filename = findChild(element, "RelativeFilename");
	if (texture_relative_filename && texture_relative_filename->first_property)
	{
		texture->relative_filename = texture_relative_filename->first_property->value;
	}
	return texture;
}

struct OptionalError<Object*> parseLight(Scene& scene, const Element& element, Allocator& allocator)
{
	LightImpl* light = allocator.allocate<LightImpl>(scene, element);

	light->lightType = static_cast<Light::LightType>(resolveEnumProperty(*light, "LightType", (int)Light::LightType::POINT));

	const Element* prop = findChild(element, "Properties70");
	if (prop) prop = prop->child;

	while (prop)
	{
		if (prop->id == "P" && prop->first_property)
		{
			if (prop->first_property->value == "Color")
			{
				light->color.r = (float)prop->getProperty(4)->getValue().toDouble();
				light->color.g = (float)prop->getProperty(5)->getValue().toDouble();
				light->color.b = (float)prop->getProperty(6)->getValue().toDouble();
			}
			if (prop->first_property->value == "ShadowColor")
			{
				light->shadowColor.r = (float)prop->getProperty(4)->getValue().toDouble();
				light->shadowColor.g = (float)prop->getProperty(5)->getValue().toDouble();
				light->shadowColor.b = (float)prop->getProperty(6)->getValue().toDouble();
			}
			else if (prop->first_property->value == "CastShadows")
			{
				light->castShadows = prop->getProperty(4)->getValue().toBool();
			}
			else if (prop->first_property->value == "InnerAngle")
			{
				light->innerAngle = (float)prop->getProperty(4)->getValue().toDouble();
			}
			else if (prop->first_property->value == "OuterAngle")
			{
				light->outerAngle = (float)prop->getProperty(4)->getValue().toDouble();
			}
			else if (prop->first_property->value == "Intensity")
			{
				light->intensity = (float)prop->getProperty(4)->getValue().toDouble();
			}
		}
		prop = prop->sibling;
	}

	scene.m_lights.push_back(light);
	return light;
}

struct OptionalError<Object*> parseCamera(Scene& scene, const Element& element, Allocator& allocator)
{
	CameraImpl* camera = allocator.allocate<CameraImpl>(scene, element);

	camera->projectionType = static_cast<Camera::ProjectionType>(resolveEnumProperty(*camera, "ProjectionType", (int)Camera::ProjectionType::PERSPECTIVE));
	camera->apertureMode = static_cast<Camera::ApertureMode>(resolveEnumProperty(*camera, "ApertureMode", (int)Camera::ApertureMode::HORIZANDVERT));
	camera->gateFit = static_cast<Camera::GateFit>(resolveEnumProperty(*camera, "GateFit", (int)Camera::GateFit::HORIZONTAL));

	const Element* prop = findChild(element, "Properties70");
	if (prop) prop = prop->child;

	while (prop)
	{
		if (prop->id == "P" && prop->first_property)
		{
			if (prop->first_property->value == "InterestPosition")
			{
				camera->interestPosition.x = (float)prop->getProperty(4)->getValue().toDouble();
				camera->interestPosition.y = (float)prop->getProperty(5)->getValue().toDouble();
				camera->interestPosition.z = (float)prop->getProperty(6)->getValue().toDouble();
			}
			else if (prop->first_property->value == "BackgroundColor")
			{
				camera->backgroundColor.x = (float)prop->getProperty(4)->getValue().toDouble();
				camera->backgroundColor.y = (float)prop->getProperty(5)->getValue().toDouble();
				camera->backgroundColor.z = (float)prop->getProperty(6)->getValue().toDouble();
			}
			else if (prop->first_property->value == "FocalLength")
			{
				camera->focalLength = prop->getProperty(4)->getValue().toDouble();
			}
			else if (prop->first_property->value == "FocusDistance")
			{
				camera->focusDistance = prop->getProperty(4)->getValue().toDouble();
			}
			else if (prop->first_property->value == "FilmAspectRatio")
			{
				camera->filmAspectRatio = prop->getProperty(4)->getValue().toDouble();
			}
			else if (prop->first_property->value == "FilmWidth")
			{
				camera->filmWidth = prop->getProperty(4)->getValue().toDouble();
			}
			else if (prop->first_property->value == "FilmHeight")
			{
				camera->filmHeight = prop->getProperty(4)->getValue().toDouble();
			}
			else if (prop->first_property->value == "AspectHeight")
			{
				camera->aspectHeight = prop->getProperty(4)->getValue().toDouble();
			}
			else if (prop->first_property->value == "AspectWidth")
			{
				camera->aspectWidth = prop->getProperty(4)->getValue().toDouble();
			}
			else if (prop->first_property->value == "AutoComputeClipPanes")
			{
				camera->autoComputeClipPanes = prop->getProperty(4)->getValue().toBool();
			}
			else if (prop->first_property->value == "NearPlane")
			{
				camera->nearPlane = prop->getProperty(4)->getValue().toDouble();
			}
			else if (prop->first_property->value == "FarPlane")
			{
				camera->farPlane = prop->getProperty(4)->getValue().toDouble();
			}
		}
		prop = prop->sibling;
	}

	camera->CalculateFOV();
	scene.m_cameras.push_back(camera);
	return camera;
}

static bool toObjectID(const Scene& scene, const Property* property, u64* out) {
	if (!property) return false;
	if (isString(property)) {
		if (property->value == "Scene") return 0;
		std::string tmp((const char*)property->value.begin, property->value.end - property->value.begin);
		auto iter = scene.m_fake_ids.find(tmp);
		if (iter != scene.m_fake_ids.end()) {
			*out = iter->second;
			return true;
		}

		return false;
	}

	*out = property->value.toU64();
	return true;
}


static u64 toObjectID(Scene& scene, const Property* property) {
	if (isString(property)) {
		if (property->value == "Scene") return 0;
		std::string tmp((const char*)property->value.begin, property->value.end - property->value.begin);
		auto iter = scene.m_fake_ids.find(tmp);
		if (iter != scene.m_fake_ids.end()) return iter->second;

		scene.m_fake_ids.emplace(std::move(tmp), scene.m_fake_ids.size() + 1); // ID 0 is reserved for root
		return scene.m_fake_ids.size();
	}

	return property->value.toU64();
}

struct OptionalError<Object*> parsePose(Scene& scene, const Element& element, Allocator& allocator)
{
	PoseImpl* pose = allocator.allocate<PoseImpl>(scene, element);
	const Element* pose_node = findChild(element, "PoseNode");
	if (pose_node) {
		const Element* node = findChild(*pose_node, "Node");
		const Element* matrix = findChild(*pose_node, "Matrix");

		if (matrix && matrix->first_property) {
			if (!matrix->first_property->getValues(&pose->matrix.m[0], sizeof(pose->matrix))) {
				return Error("Failed to parse pose");
			}
		}
		pose->node_id = toObjectID(scene, node->first_property);
	}
	return pose;
}

static OptionalError<Object*> parseCluster(const Scene& scene, const Element& element, Allocator& allocator)
{
	ClusterImpl* obj = allocator.allocate<ClusterImpl>(scene, element);

	const Element* transform_link = findChild(element, "TransformLink");
	if (transform_link && transform_link->first_property)
	{
		if (!transform_link->first_property->getValues(&obj->transform_link_matrix.m[0], sizeof(obj->transform_link_matrix)))
		{
			return Error("Failed to parse TransformLink");
		}
	}
	const Element* transform = findChild(element, "Transform");
	if (transform && transform->first_property)
	{
		if (!transform->first_property->getValues(&obj->transform_matrix.m[0], sizeof(obj->transform_matrix)))
		{
			return Error("Failed to parse Transform");
		}
	}

	return obj;
}


static OptionalError<Object*> parseNodeAttribute(const Scene& scene, const Element& element, Allocator& allocator)
{
	NodeAttributeImpl* obj = allocator.allocate<NodeAttributeImpl>(scene, element);
	const Element* type_flags = findChild(element, "TypeFlags");
	if (type_flags && type_flags->first_property)
	{
		obj->attribute_type = type_flags->first_property->value;
	}
	return obj;
}


static OptionalError<Object*> parseMaterial(const Scene& scene, const Element& element, Allocator& allocator)
{
	MaterialImpl* material = allocator.allocate<MaterialImpl>(scene, element);
	const char* property_id = "P";
	int property_offset = 4;
	const Element* prop = findChild(element, "Properties70");
	if (!prop) {
		property_id = "Property";
		property_offset = 3;
		prop = findChild(element, "Properties60");
	}
	material->diffuse_color = {1, 1, 1};
	if (prop) prop = prop->child;
	while (prop)
	{
		if (prop->id == property_id && prop->first_property)
		{
			if (prop->first_property->value == "DiffuseColor")
			{
				material->diffuse_color.r = (float)prop->getProperty(property_offset + 0)->getValue().toDouble();
				material->diffuse_color.g = (float)prop->getProperty(property_offset + 1)->getValue().toDouble();
				material->diffuse_color.b = (float)prop->getProperty(property_offset + 2)->getValue().toDouble();
			}
			else if (prop->first_property->value == "SpecularColor")
			{
				material->specular_color.r = (float)prop->getProperty(property_offset + 0)->getValue().toDouble();
				material->specular_color.g = (float)prop->getProperty(property_offset + 1)->getValue().toDouble();
				material->specular_color.b = (float)prop->getProperty(property_offset + 2)->getValue().toDouble();
			}
			else if (prop->first_property->value == "Shininess")
			{
				material->shininess = (float)prop->getProperty(property_offset)->getValue().toDouble();
			}
			else if (prop->first_property->value == "ShininessExponent")
			{
				material->shininess_exponent = (float)prop->getProperty(property_offset)->getValue().toDouble();
			}
			else if (prop->first_property->value == "ReflectionColor")
			{
				material->reflection_color.r = (float)prop->getProperty(property_offset + 0)->getValue().toDouble();
				material->reflection_color.g = (float)prop->getProperty(property_offset + 1)->getValue().toDouble();
				material->reflection_color.b = (float)prop->getProperty(property_offset + 2)->getValue().toDouble();
			}
			else if (prop->first_property->value == "AmbientColor")
			{
				material->ambient_color.r = (float)prop->getProperty(property_offset + 0)->getValue().toDouble();
				material->ambient_color.g = (float)prop->getProperty(property_offset + 1)->getValue().toDouble();
				material->ambient_color.b = (float)prop->getProperty(property_offset + 2)->getValue().toDouble();
			}
			else if (prop->first_property->value == "EmissiveColor")
			{
				material->emissive_color.r = (float)prop->getProperty(property_offset + 0)->getValue().toDouble();
				material->emissive_color.g = (float)prop->getProperty(property_offset + 1)->getValue().toDouble();
				material->emissive_color.b = (float)prop->getProperty(property_offset + 2)->getValue().toDouble();
			}
			else if (prop->first_property->value == "ReflectionFactor")
			{
				material->reflection_factor = (float)prop->getProperty(property_offset)->getValue().toDouble();
			}
			else if (prop->first_property->value == "BumpFactor")
			{
				material->bump_factor = (float)prop->getProperty(property_offset)->getValue().toDouble();
			}
			else if (prop->first_property->value == "AmbientFactor")
			{
				material->ambient_factor = (float)prop->getProperty(property_offset)->getValue().toDouble();
			}
			else if (prop->first_property->value == "DiffuseFactor")
			{
				material->diffuse_factor = (float)prop->getProperty(property_offset)->getValue().toDouble();
			}
			else if (prop->first_property->value == "SpecularFactor")
			{
				material->specular_factor = (float)prop->getProperty(property_offset)->getValue().toDouble();
			}
			else if (prop->first_property->value == "EmissiveFactor")
			{
				material->emissive_factor = (float)prop->getProperty(property_offset)->getValue().toDouble();
			}
		}
		prop = prop->sibling;
	}
	return material;
}

template <typename T> const char* fromString(const char* str, const char* end, T* val);
template <> const char* fromString<int>(const char* str, const char* end, int* val)
{
	*val = atoi(str);
	const char* iter = str;
	while (iter < end && *iter != ',') ++iter;
	if (iter < end) ++iter; // skip ','
	return (const char*)iter;
}


template <> const char* fromString<u64>(const char* str, const char* end, u64* val)
{
	*val = strtoull(str, nullptr, 10);
	const char* iter = str;
	while (iter < end && *iter != ',') ++iter;
	if (iter < end) ++iter; // skip ','
	return (const char*)iter;
}


template <> const char* fromString<i64>(const char* str, const char* end, i64* val)
{
	*val = atoll(str);
	const char* iter = str;
	while (iter < end && *iter != ',') ++iter;
	if (iter < end) ++iter; // skip ','
	return (const char*)iter;
}


template <> const char* fromString<double>(const char* str, const char* end, double* val)
{
	*val = atof(str);
	const char* iter = str;
	while (iter < end && *iter != ',') ++iter;
	if (iter < end) ++iter; // skip ','
	return (const char*)iter;
}


template <> const char* fromString<float>(const char* str, const char* end, float* val)
{
	*val = (float)atof(str);
	const char* iter = str;
	while (iter < end && *iter != ',') ++iter;
	if (iter < end) ++iter; // skip ','
	return (const char*)iter;
}


const char* fromString(const char* str, const char* end, double* val, int count)
{
	const char* iter = str;
	for (int i = 0; i < count; ++i)
	{
		*val = atof(iter);
		++val;
		while (iter < end && *iter != ',') ++iter;
		if (iter < end) ++iter; // skip ','

		if (iter == end) return iter;
	}
	return (const char*)iter;
}

const char* fromString(const char* str, const char* end, float* val, int count)
{
	const char* iter = str;
	for (int i = 0; i < count; ++i)
	{
		*val = (float)atof(iter);
		++val;
		while (iter < end && *iter != ',') ++iter;
		if (iter < end) ++iter; // skip ','

		if (iter == end) return iter;
	}
	return (const char*)iter;
}

template <> const char* fromString<DVec2>(const char* str, const char* end, DVec2* val)
{
	return fromString(str, end, &val->x, 2);
}

template <> const char* fromString<FVec2>(const char* str, const char* end, FVec2* val)
{
	return fromString(str, end, &val->x, 2);
}

template <> const char* fromString<FVec3>(const char* str, const char* end, FVec3* val)
{
	return fromString(str, end, &val->x, 3);
}

template <> const char* fromString<DVec3>(const char* str, const char* end, DVec3* val)
{
	return fromString(str, end, &val->x, 3);
}

template <> const char* fromString<DVec4>(const char* str, const char* end, DVec4* val)
{
	return fromString(str, end, &val->x, 4);
}

template <> const char* fromString<FVec4>(const char* str, const char* end, FVec4* val)
{
	return fromString(str, end, &val->x, 4);
}

template <> const char* fromString<DMatrix>(const char* str, const char* end, DMatrix* val)
{
	return fromString(str, end, &val->m[0], 16);
}


template <typename T> static bool parseMemoryText(const Property& property, T* out, int max_size_bytes) {
	const u8* iter = property.value.begin;
	int max_count = max_size_bytes / sizeof(T);
	int count = 0;
	while (iter < property.value.end) {
		if (count >= max_count) return false;
		T val;
		iter = (const u8*)fromString<T>((const char*)iter, (const char*)property.value.end, &val);
		out[count] = val;
		++count;
	}
	return true;
}

template <typename T> static bool parseMemoryLinked(const Property& property, T* out, int max_size_bytes) {
	assert(out);
	assert(property.value.is_binary);

	int elem_size = 1;
	switch (property.type) {
		case 'L': elem_size = 8; break;
		case 'D': elem_size = 8; break;
		case 'F': elem_size = 4; break;
		case 'I': elem_size = 4; break;
		default: return false;
	}

	if (sizeof(T) % elem_size != 0) return false;

	const Property* p = &property;
	int count = 0;
	int max_count = max_size_bytes / sizeof(T);
	while (p) {
		T tmp;
		if (count == max_count) return false;
		for (u32 i = 0; i < sizeof(T) / elem_size; ++i) {
			if (!p) return false;
			if (p->type != property.type) return false;

			memcpy((u8*)&tmp + elem_size * i, p->value.begin, elem_size);
			p = p->next;
		}
		out[count] = tmp;
		++count;
	}

	return true;
}

template <typename T> struct TElemType;
template <> struct TElemType<float> { using Type = float; };
template <> struct TElemType<double> { using Type = double; };
template <> struct TElemType<int> { using Type = int; };
template <> struct TElemType<i64> { using Type = i64; };
template <> struct TElemType<u64> { using Type = u64; };
template <> struct TElemType<DVec2> { using Type = double; };
template <> struct TElemType<DVec3> { using Type = double; };
template <> struct TElemType<DVec4> { using Type = double; };
template <> struct TElemType<FVec2> { using Type = float; };
template <> struct TElemType<FVec3> { using Type = float; };
template <> struct TElemType<FVec4> { using Type = float; };

template <typename T> bool typeMatch(u8 type);
template <> bool typeMatch<int>(u8 type) { return type == Property::INTEGER || type == Property::ARRAY_INT; }
template <> bool typeMatch<float>(u8 type) { return type == Property::FLOAT || type == Property::ARRAY_FLOAT; }
template <> bool typeMatch<double>(u8 type) { return type == Property::DOUBLE || type == Property::ARRAY_DOUBLE; }
template <> bool typeMatch<u64>(u8 type) { return type == Property::LONG || type == Property::ARRAY_LONG; }
template <> bool typeMatch<i64>(u8 type) { return type == Property::LONG || type == Property::ARRAY_LONG; }

template <typename T>
static bool parseMemory(const Property& property, T* out, int max_size_bytes) {
	assert(out);
	const u32 count = property.getCount();
	if (count == 0) return true;

	if (!property.value.is_binary) return parseMemoryText(property, out, max_size_bytes);

	if (!typeMatch<typename TElemType<T>::Type>(property.type))
		return false;
	int elem_size = 1;
	switch (property.type) {
		case 'l': elem_size = 8; break;
		case 'd': elem_size = 8; break;
		case 'f': elem_size = 4; break;
		case 'i': elem_size = 4; break;
		case 'L':
		case 'D':
		case 'F':
		case 'I':
			return parseMemoryLinked(property, out, max_size_bytes); 
		default: return false;
	}
	if (count * elem_size != max_size_bytes) return false;
	if (sizeof(T) % elem_size != 0) return false;

	const u8* data = property.value.begin + sizeof(u32) * 3;
	if (data > property.value.end) return false;

	u32 enc = *(const u32*)(property.value.begin + 4);
	u32 len = *(const u32*)(property.value.begin + 8);

	if (enc == 0) {
		if ((int)len > max_size_bytes) return false;
		if (data + len > property.value.end) return false;
		memcpy(out, data, len);
		return true;
	}
	else if (enc == 1) {
		if (int(elem_size * count) > max_size_bytes) return false;
		return decompress(data, len, (u8*)out, elem_size * count);
	}

	return false;
}

template <typename T> static void parseTextArray(const Property& property, std::vector<T>* out)
{
	out->clear();
	const u8* iter = property.value.begin;
	while (iter < property.value.end)
	{
		T val;
		iter = (const u8*)fromString<T>((const char*)iter, (const char*)property.value.end, &val);
		out->push_back(val);
	}
}

template <typename T> static bool parseBinaryArrayLinked(const Property& property, std::vector<T>* out)
{
	assert(out);
	assert(property.value.is_binary);

	int elem_size = 1;
	switch (property.type)
	{
		case 'L': elem_size = 8; break;
		case 'D': elem_size = 8; break;
		case 'F': elem_size = 4; break;
		case 'I': elem_size = 4; break;
		default: return false;
	}

	if (sizeof(T) % elem_size != 0) return false;

	const Property* p = &property;
	while (p) {
		T tmp;
		for (u32 i = 0; i < sizeof(T) / elem_size; ++i) {
			if (!p) return false;
			if (p->type != property.type) return false;

			memcpy((u8*)&tmp + elem_size * i, p->value.begin, elem_size);
			p = p->next;
		}
		out->push_back(tmp);
	}

	return true;
}

template <typename T> static bool parseArray(const Property& property, std::vector<T>* out) {
	assert(out);
	if (!property.value.is_binary) {
		parseTextArray(property, out);
		return true;
	}

	if (!typeMatch<typename TElemType<T>::Type>(property.type)) return false;

	int elem_size = 1;
	switch (property.type)
	{
		case 'l': elem_size = 8; break;
		case 'd': elem_size = 8; break;
		case 'f': elem_size = 4; break;
		case 'i': elem_size = 4; break;
		case 'L':
		case 'D':
		case 'F':
		case 'I':
			return parseBinaryArrayLinked(property, out); 
		default: return false;
	}
	u32 count = property.getCount();
	out->resize(count * elem_size / sizeof(T));
	if (count == 0) return true;

	return parseMemory(property, out->data(), int(sizeof((*out)[0]) * out->size()));
}

template <typename T> static bool parseVecData(Property& property, std::vector<T>* out_vec) {
	using ElemType = typename TElemType<T>::Type;
	assert(out_vec);
	if (!property.value.is_binary) {
		parseTextArray(property, out_vec);
		return true;
	}

	if (typeMatch<ElemType>(property.type)) return parseArray(property, out_vec);

	if (property.type == 'f' || property.type == 'F') {
		std::vector<float> tmp;
		if (!parseArray(property, &tmp)) return false;
		int elem_count = sizeof((*out_vec)[0]) / sizeof(ElemType);
		out_vec->resize(tmp.size() / elem_count);
		ElemType* out = (ElemType*)out_vec->data();
		for (int i = 0, c = (int)tmp.size(); i < c; ++i) {
			out[i] = static_cast<ElemType>(tmp[i]);
		}
		return true;
	}

	if (property.type == 'd' || property.type == 'D') {
		std::vector<double> tmp;
		if (!parseArray(property, &tmp)) return false;
		int elem_count = sizeof((*out_vec)[0]) / sizeof(ElemType);
		out_vec->resize(tmp.size() / elem_count);
		auto* out = (ElemType*)out_vec->data();
		for (int i = 0, c = (int)tmp.size(); i < c; ++i) {
			out[i] = static_cast<ElemType>(tmp[i]);
		}
		return true;
	}

	return false;
}

template <typename T> static bool parseVertexData(const Element& element, const char* name, const char* index_name, T& out, std::vector<ParseDataJob>& jobs) {
	const Element* data_element = findChild(element, name);
	if (!data_element || !data_element->first_property) return false;

	const Element* mapping_element = findChild(element, "MappingInformationType");
	const Element* reference_element = findChild(element, "ReferenceInformationType");
	if (mapping_element && mapping_element->first_property) {
		if (mapping_element->first_property->value == "ByPolygonVertex") {
			out.mapping = VertexDataMapping::BY_POLYGON_VERTEX;
		} else if (mapping_element->first_property->value == "ByPolygon") {
			out.mapping = VertexDataMapping::BY_POLYGON;
		} else if (mapping_element->first_property->value == "ByVertice" || mapping_element->first_property->value == "ByVertex") {
			out.mapping = VertexDataMapping::BY_VERTEX;
		} else {
			return false;
		}
	}
	if (reference_element && reference_element->first_property) {
		if (reference_element->first_property->value == "IndexToDirect") {
			const Element* indices_element = findChild(element, index_name);
			if (indices_element && indices_element->first_property) {
				if (!pushJob(jobs, *indices_element->first_property, out.indices)) return false;
			}
		} else if (reference_element->first_property->value != "Direct") {
			return false;
		}
	}
	return pushJob(jobs, *data_element->first_property, out.values);
}

static bool parseDouble(Property& property, double* out)
{
	assert(out);
	if (property.value.is_binary)
	{
		int elem_size = 1;
		switch (property.type)
		{
			case 'D': elem_size = 8; break;
			case 'F': elem_size = 4; break;
			default: return false;
		}
		const u8* data = property.value.begin;
		if (data > property.value.end) return false;
		memcpy(out, data, elem_size);
		return true;
	}
	else
	{
		fromString<double>((const char*)property.value.begin, (const char*)property.value.end, out);
		return true;
	}
}

static OptionalError<Object*> parseAnimationCurve(const Scene& scene, const Element& element, Allocator& allocator)
{
	AnimationCurveImpl* curve = allocator.allocate<AnimationCurveImpl>(scene, element);

	const Element* times = findChild(element, "KeyTime");
	const Element* values = findChild(element, "KeyValueFloat");

	if (times && times->first_property)
	{
		curve->times.resize(times->first_property->getCount());
		if (!times->first_property->getValues(&curve->times[0], (int)curve->times.size() * sizeof(curve->times[0])))
		{
			return Error("Invalid animation curve");
		}
	}

	if (values && values->first_property)
	{
		curve->values.resize(values->first_property->getCount());
		if (!values->first_property->getValues(&curve->values[0], (int)curve->values.size() * sizeof(curve->values[0])))
		{
			return Error("Invalid animation curve");
		}
	}

	if (curve->times.size() != curve->values.size()) return Error("Invalid animation curve");

	return curve;
}

static OptionalError<Object*> parseGeometry(const Element& element, GeometryImpl& geom, std::vector<ParseDataJob> &jobs, Allocator& allocator) {
	assert(element.first_property);

	const Element* vertices_element = findChild(element, "Vertices");
	if (!vertices_element || !vertices_element->first_property)
	{
		return &geom;
	}

	const Element* polys_element = findChild(element, "PolygonVertexIndex");
	if (!polys_element || !polys_element->first_property) return Error("Indices missing");

	if (!pushJob(jobs, *vertices_element->first_property, geom.positions.values)) return Error("Invalid vertices");
	if (!pushJob(jobs, *polys_element->first_property, geom.positions.indices)) return Error("Invalid vertices");

	if (!parseGeometryMaterials(geom, element, jobs)) return Error("Invalid materials");
	if (!parseGeometryUVs(geom, element, jobs)) return Error("Invalid vertex attributes");
	if (!parseGeometryTangents(geom, element, jobs)) return Error("Invalid vertex attributes");
	if (!parseGeometryColors(geom, element, jobs)) return Error("Invalid vertex attributes");
	if (!parseGeometryNormals(geom, element, jobs)) return Error("Invalid vertex attributes");

	return &geom;
}


bool ShapeImpl::postprocess(GeometryImpl& geom, Allocator& allocator) {
	const Element* vertices_element = findChild((const Element&)element, "Vertices");
	const Element* normals_element = findChild((const Element&)element, "Normals");
	const Element* indexes_element = findChild((const Element&)element, "Indexes");
	if (!vertices_element || !vertices_element->first_property ||
		!indexes_element || !indexes_element->first_property)
	{
		return false;
	}

	if (!parseVecData(*vertices_element->first_property, &vertices)) return false;
	if (normals_element && !parseVecData(*normals_element->first_property, &normals)) return false;
	if (!parseVecData(*indexes_element->first_property, &indices)) return false;

	return true;
}

static bool parseConnections(const Element& root, Scene& scene)
{
	const Element* connections = findChild(root, "Connections");
	if (!connections) return true;

	scene.m_connections.reserve(1024);
	const Element* connection = connections->child;
	while (connection)
	{
		if (!isString(connection->first_property) || !connection->first_property->next || !connection->first_property->next->next)
		{
			Error::s_message = "Invalid connection";
			return false;
		}

		Scene::Connection c;
		c.from_object = toObjectID(scene, connection->first_property->next);
		if (connection->first_property->value == "OO")
		{
			c.type = Scene::Connection::OBJECT_OBJECT;
			c.to_object = toObjectID(scene, connection->first_property->next->next);
		}
		else if (connection->first_property->value == "OP")
		{
			if (!connection->first_property->next->next->next)
			{
				Error::s_message = "Invalid connection";
				return false;
			}
			c.type = Scene::Connection::OBJECT_PROPERTY;
			c.to_object = toObjectID(scene, connection->first_property->next->next);
			c.to_property = connection->first_property->next->next->next->value;
		}
		else if (connection->first_property->value == "PO")
		{
			if (!connection->first_property->next->next->next)
			{
				Error::s_message = "Invalid connection";
				return false;
			}
			c.type = Scene::Connection::PROPERTY_OBJECT;
			c.from_property = connection->first_property->next->next->value;
			c.to_object = toObjectID(scene, connection->first_property->next->next->next);
		}
		else if (connection->first_property->value == "PP")
		{
			if (!connection->first_property->next->next->next->next)
			{
				Error::s_message = "Invalid connection";
				return false;
			}
			c.type = Scene::Connection::PROPERTY_PROPERTY;
			c.from_property = connection->first_property->next->next->value;
			c.to_object = toObjectID(scene, connection->first_property->next->next->next);
			c.to_property = connection->first_property->next->next->next->next->value;
		}
		else
		{
			assert(false);
			Error::s_message = "Not supported";
			return false;
		}
		scene.m_connections.push_back(c);

		connection = connection->sibling;
	}
	return true;
}


static bool parseTakes(Scene& scene)
{
	const Element* takes = findChild((const Element&)*scene.getRootElement(), "Takes");
	if (!takes) return true;

	const Element* object = takes->child;
	while (object)
	{
		if (object->id == "Take")
		{
			if (!isString(object->first_property))
			{
				Error::s_message = "Invalid name in take";
				return false;
			}

			TakeInfo take;
			take.name = object->first_property->value;
			const Element* filename = findChild(*object, "FileName");
			if (filename)
			{
				if (!isString(filename->first_property))
				{
					Error::s_message = "Invalid filename in take";
					return false;
				}
				take.filename = filename->first_property->value;
			}
			const Element* local_time = findChild(*object, "LocalTime");
			if (local_time)
			{
				if (!isLong(local_time->first_property) || !isLong(local_time->first_property->next))
				{
					Error::s_message = "Invalid local time in take";
					return false;
				}

				take.local_time_from = fbxTimeToSeconds(local_time->first_property->value.toI64());
				take.local_time_to = fbxTimeToSeconds(local_time->first_property->next->value.toI64());
			}
			const Element* reference_time = findChild(*object, "ReferenceTime");
			if (reference_time)
			{
				if (!isLong(reference_time->first_property) || !isLong(reference_time->first_property->next))
				{
					Error::s_message = "Invalid reference time in take";
					return false;
				}

				take.reference_time_from = fbxTimeToSeconds(reference_time->first_property->value.toI64());
				take.reference_time_to = fbxTimeToSeconds(reference_time->first_property->next->value.toI64());
			}

			scene.m_take_infos.push_back(take);
		}

		object = object->sibling;
	}

	return true;
}


static float getFramerateFromTimeMode(FrameRate time_mode, float custom_frame_rate)
{
	switch (time_mode)
	{
		case FrameRate_DEFAULT: return 14;
		case FrameRate_120: return 120;
		case FrameRate_100: return 100;
		case FrameRate_60: return 60;
		case FrameRate_50: return 50;
		case FrameRate_48: return 48;
		case FrameRate_30: return 30;
		case FrameRate_30_DROP: return 30;
		case FrameRate_NTSC_DROP_FRAME: return 29.9700262f;
		case FrameRate_NTSC_FULL_FRAME: return 29.9700262f;
		case FrameRate_PAL: return 25;
		case FrameRate_CINEMA: return 24;
		case FrameRate_1000: return 1000;
		case FrameRate_CINEMA_ND: return 23.976f;
		case FrameRate_CUSTOM: return custom_frame_rate;
	}
	return -1;
}


static void parseGlobalSettings(const Element& root, Scene* scene)
{
	const Element* settings = findChild(root, "GlobalSettings");
	if (!settings) return;

	bool is_p60 = false;
	const Element* props = findChild(*settings, "Properties70");
	if (!props) {
		is_p60 = true;
		props = findChild(*settings, "Properties60");
		if (!props) return;
	}

	for (Element* node = props->child; node; node = node->sibling) {
		if (!node->first_property) continue;

		#define get_property(name, field, type, getter) if(node->first_property->value == name) \
		{ \
			IElementProperty* prop = node->getProperty(scene->version <= 6100 ? 3 : 4); \
			if (prop) \
			{ \
				DataView value = prop->getValue(); \
				scene->m_settings.field = (type)value.getter(); \
			} \
		}

		#define get_time_property(name, field, type, getter) if(node->first_property->value == name) \
		{ \
			IElementProperty* prop = node->getProperty(scene->version <= 6100 ? 3 : 4); \
			if (prop) \
			{ \
				DataView value = prop->getValue(); \
				scene->m_settings.field = fbxTimeToSeconds((type)value.getter()); \
			} \
		}

		get_property("UpAxis", UpAxis, UpVector, toInt);
		get_property("UpAxisSign", UpAxisSign, int, toInt);
		get_property("FrontAxis", FrontAxis, int, toInt);
		get_property("FrontAxisSign", FrontAxisSign, int, toInt);
		get_property("CoordAxis", CoordAxis, CoordSystem, toInt);
		get_property("CoordAxisSign", CoordAxisSign, int, toInt);
		get_property("OriginalUpAxis", OriginalUpAxis, int, toInt);
		get_property("OriginalUpAxisSign", OriginalUpAxisSign, int, toInt);
		get_property("UnitScaleFactor", UnitScaleFactor, float, toDouble);
		get_property("OriginalUnitScaleFactor", OriginalUnitScaleFactor, float, toDouble);
		get_time_property("TimeSpanStart", TimeSpanStart, u64, toU64);
		get_time_property("TimeSpanStop", TimeSpanStop, u64, toU64);
		get_property("TimeMode", TimeMode, FrameRate, toInt);
		get_property("CustomFrameRate", CustomFrameRate, float, toDouble);

		#undef get_property
		#undef get_time_property

		scene->m_scene_frame_rate = getFramerateFromTimeMode(scene->m_settings.TimeMode, scene->m_settings.CustomFrameRate);
	}
}

void sync_job_processor(JobFunction fn, void*, void* data, u32 size, u32 count) {
	u8* ptr = (u8*)data;
	for(u32 i = 0; i < count; ++i) {
		fn(ptr);
		ptr += size;
	}
}

static bool parseObjects(const Element& root, Scene& scene, u16 flags, Allocator& allocator, JobProcessor job_processor, void* job_user_ptr) {
	if (!job_processor) job_processor = &sync_job_processor;

	const bool ignore_geometry = (flags & (u16)LoadFlags::IGNORE_GEOMETRY) != 0;
	const bool ignore_blend_shapes = (flags & (u16)LoadFlags::IGNORE_BLEND_SHAPES) != 0;
	const bool ignore_cameras = (flags & (u16)LoadFlags::IGNORE_CAMERAS) != 0;
	const bool ignore_lights = (flags & (u16)LoadFlags::IGNORE_LIGHTS) != 0;
	const bool ignore_textures = (flags & (u16)LoadFlags::IGNORE_TEXTURES) != 0;
	const bool ignore_skin = (flags & (u16)LoadFlags::IGNORE_SKIN) != 0;
	const bool ignore_bones = (flags & (u16)LoadFlags::IGNORE_BONES) != 0;
	const bool ignore_pivots = (flags & (u16)LoadFlags::IGNORE_PIVOTS) != 0;
	const bool ignore_animations = (flags & (u16)LoadFlags::IGNORE_ANIMATIONS) != 0;
	const bool ignore_materials = (flags & (u16)LoadFlags::IGNORE_MATERIALS) != 0;
	const bool ignore_poses = (flags & (u16)LoadFlags::IGNORE_POSES) != 0;
	const bool ignore_videos = (flags & (u16)LoadFlags::IGNORE_VIDEOS) != 0;
	const bool ignore_limbs = (flags & (u16)LoadFlags::IGNORE_LIMBS) != 0;
	const bool ignore_meshes = (flags & (u16)LoadFlags::IGNORE_MESHES) != 0;
	const bool ignore_models = (flags & (u16)LoadFlags::IGNORE_MODELS) != 0;

	const Element* objs = findChild(root, "Objects");
	if (!objs) return true;

	scene.m_root = allocator.allocate<Root>(scene, root);
	scene.m_root->id = 0;
	scene.m_object_map[0] = {&root, scene.m_root};

	const Element* object = objs->child;
	while (object)
	{
		if (object->first_property) {
			if (!isLong(object->first_property) && !isString(object->first_property))
			{
				Error::s_message = "Invalid ID";
				return false;
			}

			u64 id = toObjectID(scene, object->first_property);
			scene.m_object_map[id] = {object, nullptr};
		}
		object = object->sibling;
	}

	std::vector<ParseDataJob> jobs;

	for (auto iter : scene.m_object_map)
	{
		OptionalError<Object*> obj = nullptr;

		if (iter.second.object == scene.m_root) continue;

		if (iter.second.element->id == "Geometry" && !ignore_geometry)
		{
			Property* last_prop = iter.second.element->first_property;
			while (last_prop->next) last_prop = last_prop->next;
			if (last_prop && last_prop->value == "Mesh")
			{
				GeometryImpl* geom = allocator.allocate<GeometryImpl>(scene, *iter.second.element);
				parseGeometry(*iter.second.element, *geom, jobs, allocator);
				obj = geom;
				scene.m_geometries.push_back(geom);
			}
			else if (last_prop && last_prop->value == "Shape")
			{
				obj = allocator.allocate<ShapeImpl>(scene, *iter.second.element);
			}
		}
		else if (iter.second.element->id == "Material" && !ignore_materials)
		{
			obj = parseMaterial(scene, *iter.second.element, allocator);
		}
		else if (iter.second.element->id == "AnimationStack" && !ignore_animations)
		{
			obj = allocator.allocate<AnimationStackImpl>(scene, *iter.second.element);
			if (!obj.isError())
			{
				AnimationStackImpl* stack = (AnimationStackImpl*)obj.getValue();
				scene.m_animation_stacks.push_back(stack);
			}
		}
		else if (iter.second.element->id == "AnimationLayer" && !ignore_animations)
		{
			obj = allocator.allocate<AnimationLayerImpl>(scene, *iter.second.element);
		}
		else if (iter.second.element->id == "AnimationCurve" && !ignore_animations)
		{
			obj = parseAnimationCurve(scene, *iter.second.element, allocator);
		}
		else if (iter.second.element->id == "AnimationCurveNode" && !ignore_animations)
		{
			obj = allocator.allocate<AnimationCurveNodeImpl>(scene, *iter.second.element);
		}
		else if (iter.second.element->id == "Deformer" && !ignore_blend_shapes)
		{
			IElementProperty* class_prop = iter.second.element->getProperty(2);
			if (!class_prop) class_prop = iter.second.element->getProperty(1);

			if (class_prop)
			{
				if (class_prop->getValue() == "Cluster")
					obj = parseCluster(scene, *iter.second.element, allocator);
				else if (class_prop->getValue() == "Skin")
					obj = allocator.allocate<SkinImpl>(scene, *iter.second.element);
				else if (class_prop->getValue() == "BlendShape" && !ignore_blend_shapes)
					obj = allocator.allocate<BlendShapeImpl>(scene, *iter.second.element);
				else if (class_prop->getValue() == "BlendShapeChannel" && !ignore_blend_shapes)
					obj = allocator.allocate<BlendShapeChannelImpl>(scene, *iter.second.element);
			}
		}
		else if (iter.second.element->id == "NodeAttribute")
		{
			Property* last_prop = iter.second.element->first_property;
			while (last_prop->next) last_prop = last_prop->next;
			if (last_prop)
			{
				if (last_prop->value == "Light" && !ignore_lights)
				{
					obj = parseLight(scene, *iter.second.element, allocator);
				}
				else if (last_prop->value == "Camera" && !ignore_cameras)
				{
					obj = parseCamera(scene, *iter.second.element, allocator);
				}
			}
			else
			{
				obj = parseNodeAttribute(scene, *iter.second.element, allocator);
			}
		}
		else if (iter.second.element->id == "Model" && !ignore_models)
		{
			IElementProperty* class_prop = iter.second.element->getProperty(2);
			if (!class_prop) class_prop = iter.second.element->getProperty(1);

			if (class_prop)
			{
				if (class_prop->getValue() == "Mesh" && !ignore_meshes)
				{
					obj = parseMesh(scene, *iter.second.element, jobs, allocator);
					if (!obj.isError()) {
						Mesh* mesh = (Mesh*)obj.getValue();
						scene.m_meshes.push_back(mesh);
						obj = mesh;
					}
				}
				else if (class_prop->getValue() == "LimbNode" && !ignore_limbs)
					obj = allocator.allocate<LimbNodeImpl>(scene, *iter.second.element);
				else
					obj = allocator.allocate<NullImpl>(scene, *iter.second.element);
			}
		}
		else if (iter.second.element->id == "Texture" && !ignore_textures)
		{
			obj = parseTexture(scene, *iter.second.element, allocator);
		}
		else if (iter.second.element->id == "Video" && !ignore_videos)
		{
			parseVideo(scene, *iter.second.element, allocator);
		}
		else if (iter.second.element->id == "Pose" && !ignore_poses)
		{
			obj = parsePose(scene, *iter.second.element, allocator);
		}

		if (obj.isError()) return false;

		scene.m_object_map[iter.first].object = obj.getValue();
		if (obj.getValue())
		{
			scene.m_all_objects.push_back(obj.getValue());
			obj.getValue()->id = iter.first;
		}
	}

	if (!jobs.empty()) {
		(*job_processor)([](void* ptr){
			ParseDataJob* job = (ParseDataJob*)ptr;
			job->error = !job->f(job->property, job->data);
		}, job_user_ptr, &jobs[0], (u32)sizeof(jobs[0]), (u32)jobs.size());

		for (const ParseDataJob& job : jobs) {
			if (job.error) {
				Error::s_message = "Failed to parse data";
				return false;
			}
		}
	}

	for (const Scene::Connection& con : scene.m_connections)
	{
		if (con.type == Scene::Connection::PROPERTY_PROPERTY) continue;

		Object* parent = scene.m_object_map[con.to_object].object;
		Object* child = scene.m_object_map[con.from_object].object;
		if (!child) continue;
		if (!parent) continue;

		switch (child->getType())
		{
			case Object::Type::NODE_ATTRIBUTE:
				if (parent->node_attribute)
				{
					Error::s_message = "Invalid node attribute";
					return false;
				}
				parent->node_attribute = (NodeAttribute*)child;
				break;
			case Object::Type::ANIMATION_CURVE_NODE:
				if (parent->isNode())
				{
					AnimationCurveNodeImpl* node = (AnimationCurveNodeImpl*)child;
					node->bone = parent;
					node->bone_link_property = con.to_property;
				}
				break;
			default: break;
		}

		switch (parent->getType())
		{
			case Object::Type::MESH:
			{
				MeshImpl* mesh = (MeshImpl*)parent;
				if (child->getType() == Object::Type::SKIN)
					mesh->skin = (Skin*)child;
				else if (child->getType() == Object::Type::BLEND_SHAPE)
					mesh->blendShape = (BlendShape*)child;

				switch (child->getType())
				{
					case Object::Type::GEOMETRY:
						if (mesh->geometry)
						{
							Error::s_message = "Invalid mesh";
							return false;
						}
						mesh->geometry = (GeometryImpl*)child;
						break;
					case Object::Type::MATERIAL: mesh->materials.push_back((Material*)child); break;
					default: break;
				}
				break;
			}
			case Object::Type::SKIN:
			{
				SkinImpl* skin = (SkinImpl*)parent;
				if (child->getType() == Object::Type::CLUSTER)
				{
					ClusterImpl* cluster = (ClusterImpl*)child;
					skin->clusters.push_back(cluster);
					if (cluster->skin)
					{
						Error::s_message = "Invalid cluster";
						return false;
					}
					cluster->skin = skin;
				}
				break;
			}
			case Object::Type::BLEND_SHAPE:
			{
				BlendShapeImpl* blendShape = (BlendShapeImpl*)parent;
				if (child->getType() == Object::Type::BLEND_SHAPE_CHANNEL)
				{
					BlendShapeChannelImpl* blendShapeChannel = (BlendShapeChannelImpl*)child;
					blendShape->blendShapeChannels.push_back(blendShapeChannel);
					if (blendShapeChannel->blendShape)
					{
						Error::s_message = "Invalid blend shape";
						return false;
					}
					blendShapeChannel->blendShape = blendShape;
				}
				break;
			}
			case Object::Type::BLEND_SHAPE_CHANNEL:
			{
				BlendShapeChannelImpl* blendShapeChannel = (BlendShapeChannelImpl*)parent;
				if (child->getType() == Object::Type::SHAPE)
				{
					ShapeImpl* shape = (ShapeImpl*)child;
					blendShapeChannel->shapes.push_back(shape);
				}
				break;
			}
			case Object::Type::MATERIAL:
			{
				MaterialImpl* mat = (MaterialImpl*)parent;
				if (child->getType() == Object::Type::TEXTURE)
				{
					Texture::TextureType type = Texture::COUNT;
					if (con.to_property == "NormalMap")
						type = Texture::NORMAL;
					else if (con.to_property == "DiffuseColor")
						type = Texture::DIFFUSE;
					else if (con.to_property == "SpecularColor")
						type = Texture::SPECULAR;
					else if (con.to_property == "ShininessExponent")
						type = Texture::SHININESS;
					else if (con.to_property == "EmissiveColor")
						type = Texture::EMISSIVE;
					else if (con.to_property == "AmbientColor")
						type = Texture::AMBIENT;
					else if (con.to_property == "ReflectionFactor")
						type = Texture::REFLECTION;
					if (type == Texture::COUNT) break;

					if (mat->textures[type])
					{
						break; // This may happen for some models (eg. 2 normal maps in use)
					}

					mat->textures[type] = (Texture*)child;
				}
				break;
			}
			case Object::Type::GEOMETRY:
			{
				GeometryImpl* geom = (GeometryImpl*)parent;
				if (child->getType() == Object::Type::SKIN)
					geom->skin = (Skin*)child;
				else if (child->getType() == Object::Type::BLEND_SHAPE)
					geom->blendShape = (BlendShape*)child;
				break;
			}
			case Object::Type::CLUSTER:
			{
				ClusterImpl* cluster = (ClusterImpl*)parent;
				if (child->getType() == Object::Type::LIMB_NODE || child->getType() == Object::Type::MESH || child->getType() == Object::Type::NULL_NODE)
				{
					if (cluster->link && cluster->link != child)
					{
						Error::s_message = "Invalid cluster";
						return false;
					}

					cluster->link = child;
				}
				break;
			}
			case Object::Type::ANIMATION_LAYER:
			{
				if (child->getType() == Object::Type::ANIMATION_CURVE_NODE)
				{
					((AnimationLayerImpl*)parent)->curve_nodes.push_back((AnimationCurveNodeImpl*)child);
				}
			}
			break;
			case Object::Type::ANIMATION_CURVE_NODE:
			{
				AnimationCurveNodeImpl* node = (AnimationCurveNodeImpl*)parent;
				if (child->getType() == Object::Type::ANIMATION_CURVE)
				{
					char tmp[32];
					con.to_property.toString(tmp);
					if (strcmp(tmp, "d|X") == 0)
					{
						node->curves[0].connection = &con;
						node->curves[0].curve = (AnimationCurve*)child;
					}
					else if (strcmp(tmp, "d|Y") == 0)
					{
						node->curves[1].connection = &con;
						node->curves[1].curve = (AnimationCurve*)child;
					}
					else if (strcmp(tmp, "d|Z") == 0)
					{
						node->curves[2].connection = &con;
						node->curves[2].curve = (AnimationCurve*)child;
					}
				}
				break;
			}
			default: break;
		}
	}


	if (!ignore_geometry) {
		struct PostprocessJob {
			Object* obj;
			bool error = false;
			PostprocessJob(Object* o) : obj(o) {}
		};
		std::vector<PostprocessJob> postprocess_jobs;
		for (auto iter : scene.m_object_map)
		{
			Object* obj = iter.second.object;
			if (!obj) continue;
			switch (obj->getType()) {
				case Object::Type::CLUSTER:
				case Object::Type::GEOMETRY:
				case Object::Type::MESH:
					postprocess_jobs.push_back({obj});
					break;
				case Object::Type::BLEND_SHAPE_CHANNEL:
					if (!((BlendShapeChannelImpl*)iter.second.object)->postprocess(scene.m_allocator)) {
						Error::s_message = "Failed to postprocess blend shape channel";
						return false;
					}
					break;
				case Object::Type::POSE:
					if (!((PoseImpl*)iter.second.object)->postprocess(scene)) {
						Error::s_message = "Failed to postprocess pose";
						return false;
					}
					break;
				default: break;
			}
		}

		if (!postprocess_jobs.empty()) {
			(*job_processor)([](void* ptr){
				PostprocessJob* job = (PostprocessJob*)ptr;
				switch (job->obj->getType()) {
					case Object::Type::CLUSTER: job->error = !((ClusterImpl*)job->obj)->postprocess(); break;
					case Object::Type::GEOMETRY: job->error = !((GeometryImpl*)job->obj)->postprocess(); break;
					case Object::Type::MESH: job->error = !((MeshImpl*)job->obj)->geometry_data.postprocess(); break;
					default: break;
				}
			}, job_user_ptr, &postprocess_jobs[0], (u32)sizeof(postprocess_jobs[0]), (u32)postprocess_jobs.size());
			for (const PostprocessJob& job : postprocess_jobs) if (job.error) {
				Error::s_message = "Failed to postprocess object";
				return false;
			}
		}
	}

	return true;
}


RotationOrder Object::getRotationOrder() const
{
	// This assumes that the default rotation order is EULER_XYZ.
	return (RotationOrder) resolveEnumProperty(*this, "RotationOrder", (int) RotationOrder::EULER_XYZ);
}


DVec3 Object::getRotationOffset() const
{
	return resolveVec3Property(*this, "RotationOffset", {0, 0, 0});
}


DVec3 Object::getRotationPivot() const
{
	return resolveVec3Property(*this, "RotationPivot", {0, 0, 0});
}


DVec3 Object::getPostRotation() const
{
	return resolveVec3Property(*this, "PostRotation", {0, 0, 0});
}


DVec3 Object::getScalingOffset() const
{
	return resolveVec3Property(*this, "ScalingOffset", {0, 0, 0});
}


DVec3 Object::getScalingPivot() const
{
	return resolveVec3Property(*this, "ScalingPivot", {0, 0, 0});
}


DMatrix Object::evalLocal(const DVec3& translation, const DVec3& rotation) const
{
	return evalLocal(translation, rotation, getLocalScaling());
}


DMatrix Object::evalLocal(const DVec3& translation, const DVec3& rotation, const DVec3& scaling) const
{
	DVec3 rotation_pivot = getRotationPivot();
	DVec3 scaling_pivot = getScalingPivot();
	RotationOrder rotation_order = getRotationOrder();

	DMatrix s = makeIdentity();
	s.m[0] = scaling.x;
	s.m[5] = scaling.y;
	s.m[10] = scaling.z;

	DMatrix t = makeIdentity();
	setTranslation(translation, &t);

	DMatrix r = getRotationMatrix(rotation, rotation_order);
	DMatrix r_pre = getRotationMatrix(getPreRotation(), RotationOrder::EULER_XYZ);
	DMatrix r_post_inv = getRotationMatrix(-getPostRotation(), RotationOrder::EULER_ZYX);

	DMatrix r_off = makeIdentity();
	setTranslation(getRotationOffset(), &r_off);

	DMatrix r_p = makeIdentity();
	setTranslation(rotation_pivot, &r_p);

	DMatrix r_p_inv = makeIdentity();
	setTranslation(-rotation_pivot, &r_p_inv);

	DMatrix s_off = makeIdentity();
	setTranslation(getScalingOffset(), &s_off);

	DMatrix s_p = makeIdentity();
	setTranslation(scaling_pivot, &s_p);

	DMatrix s_p_inv = makeIdentity();
	setTranslation(-scaling_pivot, &s_p_inv);

	// http://help.autodesk.com/view/FBX/2017/ENU/?guid=__files_GUID_10CDD63C_79C1_4F2D_BB28_AD2BE65A02ED_htm
	return t * r_off * r_p * r_pre * r * r_post_inv * r_p_inv * s_off * s_p * s * s_p_inv;
}


DVec3 Object::getLocalTranslation() const
{
	return resolveVec3Property(*this, "Lcl Translation", {0, 0, 0});
}


DVec3 Object::getPreRotation() const
{
	return resolveVec3Property(*this, "PreRotation", {0, 0, 0});
}


DVec3 Object::getLocalRotation() const
{
	return resolveVec3Property(*this, "Lcl Rotation", {0, 0, 0});
}


DVec3 Object::getLocalScaling() const
{
	return resolveVec3Property(*this, "Lcl Scaling", {1, 1, 1});
}


DMatrix Object::getGlobalTransform() const
{
	const Object* parent = getParent();
	if (!parent) return evalLocal(getLocalTranslation(), getLocalRotation());

	return parent->getGlobalTransform() * evalLocal(getLocalTranslation(), getLocalRotation());
}


DMatrix Object::getLocalTransform() const
{
	return evalLocal(getLocalTranslation(), getLocalRotation(), getLocalScaling());
}


Object* Object::resolveObjectLinkReverse(Object::Type type) const
{
	u64 id;
	if (!toObjectID(scene, ((Element&)element).first_property, &id)) return nullptr;
	for (auto& connection : scene.m_connections)
	{
		if (connection.from_object == id && connection.to_object != 0)
		{
			const Scene::ObjectPair& pair = scene.m_object_map.find(connection.to_object)->second;
			Object* obj = pair.object;
			if (obj && obj->getType() == type) return obj;
		}
	}
	return nullptr;
}


const IScene& Object::getScene() const
{
	return scene;
}


Object* Object::resolveObjectLink(int idx) const
{
	u64 id = 0;
	toObjectID(scene, ((Element&)element).first_property, &id);
	for (auto& connection : scene.m_connections)
	{
		if (connection.to_object == id && connection.from_object != 0)
		{
			Object* obj = scene.m_object_map.find(connection.from_object)->second.object;
			if (obj)
			{
				if (idx == 0) return obj;
				--idx;
			}
		}
	}
	return nullptr;
}


Object* Object::resolveObjectLink(Object::Type type, const char* property, int idx) const
{
	u64 id;
	if (!toObjectID(scene, ((Element&)element).first_property, &id)) return nullptr;
	for (auto& connection : scene.m_connections)
	{
		if (connection.to_object == id && connection.from_object != 0)
		{
			Object* obj = scene.m_object_map.find(connection.from_object)->second.object;
			if (obj && obj->getType() == type)
			{
				if (property == nullptr || connection.to_property == property)
				{
					if (idx == 0) return obj;
					--idx;
				}
			}
		}
	}
	return nullptr;
}


bool Scene::finalize() {
	for (const Connection& connection : m_connections) {
		if (connection.type != Connection::OBJECT_OBJECT) continue;
		Object* to_obj = m_object_map.find(connection.to_object)->second.object;
		Object* from_obj = m_object_map.find(connection.from_object)->second.object;
		if (!from_obj) continue;
		if (!to_obj) continue;
		if (!to_obj->is_node) continue;
		from_obj->parent = to_obj;
	}

	for (Object* object : m_all_objects) {
		if (object->depth != 0xffFFffFF) continue;
		if (object->parent == object) {
			Error::s_message = "Cyclic node hierarchy";
			return false;
		}
		if (!object->parent) {
			object->depth = 0;
			continue;
		}

		object->depth = 0;
		
		Object* parent = object->parent;
		while (parent) {
			if (parent == object) {
				Error::s_message = "Cyclic node hierarchy";
				return false;
			}
			++object->depth;
			parent = parent->parent;
		}

		Object* p = object->parent;
		Object* child = object;
		while (p) {
			p->depth = child->depth - 1;
			child = p;
			p = p->parent;
		}
	}
	return true;
}

IScene* load(const u8* data, usize size, u16 flags, JobProcessor job_processor, void* job_user_ptr)
{
	std::unique_ptr<Scene> scene(new Scene());
	scene->m_data.resize(size);
	memcpy(&scene->m_data[0], data, size);

	const bool is_binary = size >= 18 && strncmp((const char*)data, "Kaydara FBX Binary", 18) == 0;
	OptionalError<Element*> root(nullptr);
	if (is_binary) {
		u32 version;
		root = tokenize(&scene->m_data[0], size, version, scene->m_allocator);
		scene->version = version;
		if (version < 6100)
		{
			Error::s_message = "Unsupported FBX file format version. Minimum supported version is 6.1";
			return nullptr;
		}
		if (root.isError())
		{
			Error::s_message = "";
			if (root.isError()) return nullptr;
		}
	}
	else {
		root = tokenizeText(&scene->m_data[0], size, scene->m_allocator);
		if (root.isError()) return nullptr;
		const ofbx::Element* header = findChild(*root.getValue(), "FBXHeaderExtension");
		if (header) {
			const ofbx::Element* version_elem = findChild(*header, "FBXVersion");
			if (version_elem->first_property) {
				scene->version = version_elem->first_property->getValue().toU32();
			}
		}
	}

	scene->m_root_element = root.getValue();
	assert(scene->m_root_element);

	// if (parseTemplates(*root.getValue()).isError()) return nullptr;
	if (!parseConnections(*root.getValue(), *scene.get())) return nullptr;
	if (!parseTakes(*scene.get())) return nullptr;
	if (!parseObjects(*root.getValue(), *scene.get(), flags, scene->m_allocator, job_processor, job_user_ptr)) return nullptr;
	parseGlobalSettings(*root.getValue(), scene.get());
	if (!scene->finalize()) return nullptr;

	return scene.release();
}


const char* getError()
{
	return Error::s_message;
}


} // namespace ofbx
