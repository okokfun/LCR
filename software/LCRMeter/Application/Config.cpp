#include "Config.hpp"
#include "file.hpp"
#include "log.h"

using ConfigEntry = struct configEntry {
	Config::WriteFunc write;
	Config::ReadFunc read;
	void *ptr;
	configEntry *next;
	uint32_t index;
};

static ConfigEntry *first;
static uint32_t cnt;

void Config::Init() {
	first = nullptr;
	cnt = 0;
}

int Config::AddParseFunctions(WriteFunc write, ReadFunc read, void *ptr) {
	cnt++;
	ConfigEntry *add = new ConfigEntry;
	add->write = write;
	add->read = read;
	add->ptr = ptr;
	add->next = nullptr;
	add->index = cnt;
	if (!first) {
		first = add;
	} else {
		// 找到配置项的末尾
		ConfigEntry *last = first;
		while (last->next) {
			last = last->next;
		}
		last->next = add;
	}
	return cnt;
}

bool Config::RemoveParseFunctions(int index) {
	if (index < 0) {
		return false;
	}
	ConfigEntry **ptrTo = &first;
	bool removed = false;
	while(*ptrTo) {
		if((*ptrTo)->index == index) {
			// remove this
			ConfigEntry *toDelete = *ptrTo;
			*ptrTo = toDelete->next;
			delete toDelete;
			removed = true;
			break;
		} else {
			ptrTo = &(*ptrTo)->next;
		}
	}
	return removed;
}

bool Config::Store(const char* filename) {
	if (File::Open(filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
		LOG(Log_Config, LevelError, "文件创建\n失败了");
		return false;
	}
	ConfigEntry *entry = first;
	bool success = true;
	while (entry) {
		if (entry->write) {
			if (!entry->write(entry->ptr)) {
				LOG(Log_Config, LevelError, "配置写入函数失败");
				success = false;
				break;
			}
			File::Write("\n");
		}
		entry = entry->next;
	}
	File::Close();
	return success;
}

bool Config::Load(const char* filename) {
	if (File::Open(filename, FA_READ | FA_OPEN_EXISTING) != FR_OK) {
		LOG(Log_Config, LevelError, "文件打开\n失败了");
		return false;
	}
	ConfigEntry *entry = first;
	bool success = true;
	while (entry) {
		if (entry->read) {
			if (!entry->read(entry->ptr)) {
				LOG(Log_Config, LevelError, "配置读取函数失败");
				success = false;
				break;
			}
		}
		entry = entry->next;
	}
	File::Close();
	return success;
}
