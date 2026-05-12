#ifdef __HAIKU__

#include "platform.h"

#include <FilePanel.h>
#include <FindDirectory.h>
#include <Looper.h>
#include <Path.h>
#include <Messenger.h>
#include <Window.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <stdio.h>
#include <compat/sys/stat.h>
#include <string.h>

extern "C" char *open_file_dialog(void);

class FileFilter: public BRefFilter
{
	public:
	bool Filter(const entry_ref *ref,
		BNode *node,
		struct stat_beos *stat,
		const char *mimeType)
	{
		int len = strlen(ref->name);
		if (S_ISDIR(stat->st_mode))
			return true;
		if (!strcmp(mimeType, "application/zip"))
			return true;
		if (!strcasecmp(ref->name, "main.lua") ||
			!strcasecmp(ref->name, "main3.lua"))
			return true;
		if (len > 3 &&
			(!strcasecmp(ref->name + len - 4, ".idf") ||
			!strcasecmp(ref->name + len - 4, ".zip")))
			return true;
		return false;
	}
};

class FileHandler: public BLooper
{
	public:

	FileHandler()
		: BLooper("filepanel listener")
	{
		lock = create_sem(0, "filepanel sem");
		path = NULL;
		Run();
	}

	~FileHandler()
	{
		delete_sem(lock);
		if (path != NULL)
			free(path);
	}
	void MessageReceived(BMessage* message)
	{
		switch(message->what)
		{
			case B_REFS_RECEIVED:
			{
				entry_ref ref;

				message->FindRef("refs", 0, &ref);
				BEntry entry(&ref, true);
				BPath xpath;
				entry.GetPath(&xpath);
				path = strdup(xpath.Path());
				break;
			}
			case B_CANCEL:
				break;
			default:
				BHandler::MessageReceived(message);
				return;
		}
		release_sem(lock);
	}

	void Wait() {
		acquire_sem(lock);
	}

	char* path;

	private:
		sem_id lock;
};

static char ret[PATH_MAX];

char *open_file_dialog(void)
{
	FileHandler *handler = new FileHandler();
	FileFilter filter;
	BMessenger messenger(handler);
	BFilePanel panel(B_OPEN_PANEL,
		&messenger,
		NULL,
		B_FILE_NODE,
		false,
		NULL,
		&filter,
		true);
	panel.Show();
	handler->Wait();
	if (handler->path == NULL) {
		messenger.SendMessage(B_QUIT_REQUESTED);
		return NULL;
	}
	snprintf(ret, sizeof(ret), "%s", handler->path);
	ret[sizeof(ret) - 1] = 0;
	messenger.SendMessage(B_QUIT_REQUESTED);
	return ret;
}

const filesystem::path show_file_picker(bool filterBoards){
    char *path = open_file_dialog();
    if (path == NULL) return std::string();
    return std::string(path);
}

extern "C" char *appdir(void);

char *appdir(void)
{
	static char dir[PATH_MAX] = "";
	if (find_directory(B_USER_SETTINGS_DIRECTORY, -1, false, dir, sizeof(dir)) != B_OK)
		sprintf(dir, "/boot/home/config/settings");
	strncat(dir, "/Instead", sizeof(dir));
	dir[sizeof(dir) - 1] = 0;
	return dir;
}
#endif