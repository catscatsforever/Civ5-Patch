----------------------------------------------------------------
-- HOTKEY MANAGER START
----------------------------------------------------------------
include( "IconSupport" );
HotkeyManagerData = Modding.OpenUserData( "IngameHotkeyManager", 1);

g_ListenInput = false;
g_InputSeqStr = {'', '', '', ''};
res = {}

local Categories = {[0] = 'InterfaceModes', 'Commands', 'Builds', 'UnitPromotions', 'Specialists', 'Controls', 'Automates', 'Missions', };

-- Universal map from legacy KB to VK keycodes
g_KeyMap = {
    KB_MOUSE_LBUTTON = {1, "VK_LBUTTON", "LMB"}, --//not used
    KB_MOUSE_RBUTTON = {2, "VK_RBUTTON", "RMB"}, --//not used
    KB_CANCEL = {},
    KB_MOUSE_MIDDLEBUTTON = {4, "VK_MBUTTON", "MMB"}, --//not used
    KB_MOUSE_XBUTTON1 = {}, --//not used
    KB_MOUSE_XBUTTON2 = {}, --//not used
    KB_UNUSED_07 = {}, --//not used
    KB_BACKSPACE = {8, "VK_BACK", "Backspace"},
    KB_TAB = {9, "VK_TAB", "TAB"},
    KB_UNUSED_0A = {},
    KB_UNUSED_0B = {},
    KB_CLEAR = {12, "VK_CLEAR", "CLEAR"},
    KB_RETURN = {13, "VK_RETURN", "Enter"},
    KB_UNUSED_0E = {},
    KB_UNUSED_0F = {},
    KB_SHIFT = {16, "VK_SHIFT", "..."},
    KB_CTRL = {17, "VK_CONTROL", "..."},
    KB_ALT = {18, "VK_MENU", "..."},
    KB_PAUSE = {19, "VK_PAUSE", "Pause Break"},
    KB_CAPSLOCK = {20, "VK_CAPITAL", "Caps Lock"},
    KB_KANA = {}, --//language key
    KB_UNUSED_16 = {},
    KB_JUNJA = {}, --//Language key
    KB_FINAL = {}, --//Language key
    KB_HANJA = {},  --//Language key
    KB_UNUSED_1A = {},
    KB_ESCAPE = {27, "VK_ESCAPE", "Esc"},
    KB_CONVERT = {}, --//language key
    KB_NONCONVERT = {}, --//language key
    KB_ACCEPT = {}, --//language key
    KB_MODECHANGE = {}, --//language key
    KB_SPACE = {32, "VK_SPACE", "Spacebar"},
    KB_PGUP = {33, "VK_PRIOR", "Page Up"},
    KB_PGDN = {34, "VK_NEXT", "Page Down"},
    KB_END = {35, "VK_END", "End"},
    KB_HOME = {36, "VK_HOME", "Home"},
    KB_LEFT = {37, "VK_LEFT", "Left Arrow"},
    KB_UP = {38, "VK_UP", "Up Arrow"},
    KB_RIGHT = {39, "VK_RIGHT", "Right Arrow"},
    KB_DOWN = {40, "VK_DOWN", "Down Arrow"},
    KB_SELECT = {41, "VK_SELECT", "SELECT"},
    KB_PRINT = {42, "VK_PRINT", "PRINT"},
    KB_EXECUTE = {43, "VK_EXECUTE ", "EXECUTE"},
    KB_PRINTSCRN = {44, "VK_SNAPSHOT", "Print Screen"},
    KB_INSERT = {45, "VK_INSERT", "Insert"},
    KB_DELETE = {46, "VK_DELETE", "Delete"},
    KB_HELP = {47, "VK_HELP", "HELP"},
    KB_0 = {48, "0", "0"},
    KB_1 = {49, "1", "1"},
    KB_2 = {50, "2", "2"},
    KB_3 = {51, "3", "3"},
    KB_4 = {52, "4", "4"},
    KB_5 = {53, "5", "5"},
    KB_6 = {54, "6", "6"},
    KB_7 = {55, "7", "7"},
    KB_8 = {56, "8", "8"},
    KB_9 = {57, "9", "9"},
    KB_UNUSED_3A = {},
    KB_UNUSED_3B = {},
    KB_UNUSED_3C = {},
    KB_UNUSED_3D = {},
    KB_UNUSED_3E = {},
    KB_UNUSED_3F = {},
    KB_UNUSED_40 = {},
    KB_A = {65, "A", "A"},
    KB_B = {66, "B", "B"},
    KB_C = {67, "C", "C"},
    KB_D = {68, "D", "D"},
    KB_E = {69, "E", "E"},
    KB_F = {70, "F", "F"},
    KB_G = {71, "G", "G"},
    KB_H = {72, "H", "H"},
    KB_I = {73, "I", "I"},
    KB_J = {74, "J", "J"},
    KB_K = {75, "K", "K"},
    KB_L = {76, "L", "L"},
    KB_M = {77, "M", "M"},
    KB_N = {78, "N", "N"},
    KB_O = {79, "O", "O"},
    KB_P = {80, "P", "P"},
    KB_Q = {81, "Q", "Q"},
    KB_R = {82, "R", "R"},
    KB_S = {83, "S", "S"},
    KB_T = {84, "T", "T"},
    KB_U = {85, "U", "U"},
    KB_V = {86, "V", "V"},
    KB_W = {87, "W", "W"},
    KB_X = {88, "X", "X"},
    KB_Y = {89, "Y", "Y"},
    KB_Z = {90, "Z", "Z"},
    KB_LWIN = {91, "VK_LWIN", "[COLOR_GREY]L WIN"},
    KB_RWIN = {92, "VK_RWIN", "[COLOR_GREY]R WIN"},
    KB_APPS = {93, "VK_APPS", "[COLOR_GREY]APPS"},
    KB_UNUSED_5E = {},
    KB_SLEEP = {},
    KB_NUMPAD0 = {96, "VK_NUMPAD0", "NUMPAD 0"},
    KB_NUMPAD1 = {97, "VK_NUMPAD1", "NUMPAD 1"},
    KB_NUMPAD2 = {98, "VK_NUMPAD2", "NUMPAD 2"},
    KB_NUMPAD3 = {99, "VK_NUMPAD3", "NUMPAD 3"},
    KB_NUMPAD4 = {100, "VK_NUMPAD4", "NUMPAD 4"},
    KB_NUMPAD5 = {101, "VK_NUMPAD5", "NUMPAD 5"},
    KB_NUMPAD6 = {102, "VK_NUMPAD6", "NUMPAD 6"},
    KB_NUMPAD7 = {103, "VK_NUMPAD7", "NUMPAD 7"},
    KB_NUMPAD8 = {104, "VK_NUMPAD8", "NUMPAD 8"},
    KB_NUMPAD9 = {105, "VK_NUMPAD9", "NUMPAD 9"},
    KB_NUMPADSTAR = {106, "VK_MULTIPLY", "NUMPAD *"},
    KB_NUMPADPLUS = {107, "VK_ADD", "NUMPAD +"},
    KB_SEPERATOR = {108, "VK_SEPARATOR", "SEPARATOR"},
    KB_NUMPADMINUS = {109, "VK_SUBTRACT", "NUMPAD -"},
    KB_NUMPADPERIOD = {110, "VK_DECIMAL", "[COLOR_GREY]NUMPAD ."},
    KB_NUMPADSLASH = {111, "VK_DIVIDE", "[COLOR_GREY]NUMPAD /"},
    KB_F1 = {112, "VK_F1", "F1"},
    KB_F2 = {113, "VK_F2", "F2"},
    KB_F3 = {114, "VK_F3", "F3"},
    KB_F4 = {115, "VK_F4", "F4"},
    KB_F5 = {116, "VK_F5", "F5"},
    KB_F6 = {117, "VK_F6", "F6"},
    KB_F7 = {118, "VK_F7", "F7"},
    KB_F8 = {119, "VK_F8", "F8"},
    KB_F9 = {120, "VK_F9", "F9"},
    KB_F10 = {121, "VK_F10", "F10"},
    KB_F11 = {122, "VK_F11", "F11"},
    KB_F12 = {123, "VK_F12", "F12"},
    KB_F13 = {124, "VK_F13", "[COLOR_GREY]F13"},
    KB_F14 = {125, "VK_F14", "[COLOR_GREY]F14"},
    KB_F15 = {126, "VK_F15", "[COLOR_GREY]F15"},
    KB_F16 = {127, "VK_F16", "[COLOR_GREY]F16"},
    KB_F17 = {128, "VK_F17", "[COLOR_GREY]F17"},
    KB_F18 = {129, "VK_F18", "[COLOR_GREY]F18"},
    KB_F19 = {130, "VK_F19", "[COLOR_GREY]F19"},
    KB_F20 = {131, "VK_F20", "[COLOR_GREY]F20"},
    KB_F21 = {132, "VK_F21", "[COLOR_GREY]F21"},
    KB_F22 = {133, "VK_F22", "[COLOR_GREY]F22"},
    KB_F23 = {134, "VK_F23", "[COLOR_GREY]F23"},
    KB_F24 = {135, "VK_F24", "[COLOR_GREY]F24"},
    KB_UNUSED_88 = {},
    KB_UNUSED_89 = {},
    KB_UNUSED_8A = {},
    KB_UNUSED_8B = {},
    KB_UNUSED_8C = {},
    KB_UNUSED_8D = {},
    KB_UNUSED_8E = {},
    KB_UNUSED_8F = {},
    KB_NUMLOCK = {144, "VK_NUMLOCK", "Num Lock"},
    KB_SCROLL = {145, "VK_SCROLL", "Scroll Lock"},
    KB_UNUSED_92_OEM = {},
    KB_UNUSED_93_OEM = {},
    KB_UNUSED_94_OEM = {},
    KB_UNUSED_95_OEM = {},
    KB_UNUSED_96_OEM = {},
    KB_UNUSED_97 = {},
    KB_UNUSED_98 = {},
    KB_UNUSED_99 = {},
    KB_UNUSED_9A = {},
    KB_UNUSED_9B = {},
    KB_UNUSED_9C = {},
    KB_UNUSED_9D = {},
    KB_UNUSED_9E = {},
    KB_UNUSED_9F = {},
    KB_LSHIFT = {160, "VK_LSHIFT", "[COLOR_GREY]L SHIFT"},
    KB_RSHIFT = {161, "VK_RSHIFT", "[COLOR_GREY]R SHIFT"},
    KB_LCTRL = {162, "VK_LCONTROL", "[COLOR_GREY]L CONTROL"},
    KB_RCTRL = {163, "VK_RCONTROL", "[COLOR_GREY]R CONTROL"},
    KB_LMENU = {164, "VK_LMENU", "[COLOR_GREY]L ALT"},
    KB_RMENU = {165, "VK_RMENU", "[COLOR_GREY]R ALT"},
    KB_BROWSER_BACK = {166, "VK_BROWSER_BACK", "[COLOR_GREY]BROWSER BACK"},
    KB_BROWSER_FORWARD = {167, "VK_BROWSER_FORWARD", "[COLOR_GREY]BROWSER FORWARD"},
    KB_BROWSER_REFRESH = {168, "VK_BROWSER_REFRESH", "[COLOR_GREY]BROWSER REFRESH"},
    KB_BROWSER_STOP = {169, "VK_BROWSER_STOP", "[COLOR_GREY]BROWSER STOP"},
    KB_BROWSER_SEARCH = {170, "VK_BROWSER_SEARCH", "[COLOR_GREY]BROWSER SEARCH"},
    KB_BROWSER_FAVORITES = {171, "VK_BROWSER_FAVORITES", "[COLOR_GREY]BROWSER FAVORITES"},
    KB_BROWSER_HOME = {172, "VK_BROWSER_HOME", "[COLOR_GREY]BROWSER HOME"},
    KB_VOLUME_MUTE = {173, "VK_VOLUME_MUTE", "[COLOR_GREY]VOLUME MUTE"},
    KB_VOLUMEDOWN = {174, "VK_VOLUME_DOWN", "VOLUME DOWN"},
    KB_VOLUMEUP = {175, "VK_VOLUME_UP", "VOLUME UP"},
    KB_MEDIA_NEXT_TRACK = {176, "VK_MEDIA_NEXT_TRACK", "[COLOR_GREY]NEXT TRACK"},
    KB_MEDIA_PREV_TRACK = {177, "VK_MEDIA_PREV_TRACK", "[COLOR_GREY]PREVIOUS TRACK"},
    KB_MEDIA_STOP = {178, "VK_MEDIA_STOP", "[COLOR_GREY]STOP MEDIA"},
    KB_MEDIA_PLAY = {179, "VK_MEDIA_PLAY_PAUSE", "[COLOR_GREY]PLAY/PAUSE MEDIA"},
    KB_LAUNCH_MAIL = {180, "VK_LAUNCH_MAIL", "[COLOR_GREY]START MAIL"},
    KB_LAUNCH_MEDIA_SELECT = {181, "VK_LAUNCH_MEDIA_SELECT", "[COLOR_GREY]SELECT MEDIA"},
    KB_LAUNCH_APP1 = {182, "VK_LAUNCH_APP1", "[COLOR_GREY]START APP1"},
    KB_LAUNCH_APP2 = {183, "VK_LAUNCH_APP2", "[COLOR_GREY]START APP2"},
    KB_UNUSED_B8 = {},
    KB_UNUSED_B9 = {},
    KB_SEMICOLON = {186, "VK_OEM_1", ";"}, --//semicolon for US keyboards
    KB_EQUALS = {187, "VK_OEM_PLUS", "="}, --//plus key in US
    KB_COMMA = {188, "VK_OEM_COMMA", ","}, --//comma key in US
    KB_MINUS = {189, "VK_OEM_MINUS", "-"}, --//minus key in US
    KB_PERIOD = {190, "VK_OEM_PERIOD", "."}, --//period key in US
    KB_SLASH = {191, "VK_OEM_2", "/"}, --//question mark key in US
    KB_GRAVE = {192, "VK_OEM_3", "~"}, --//tidle key in US

    KB_LBRACKET = {219, "VK_OEM_4", "{"}, --//left bracket
    KB_BACKSLASH = {220, "VK_OEM_5", "\\"}, --//backslash
    KB_RBRACKET = {221, "VK_OEM_6", "}"}, --//right bracket
    KB_APOSTROPHE = {222, "VK_OEM_7", "\'"}, --//quotation key
    KB_OEM_8 = {223, "VK_OEM_8", "VK_OEM_8"}, --//misc keys

    M_ = {}, --//Left mouse button
    M_ = {}, --//Right
    M_ = {}, --//Middle
    M_ = {}, --//4
    M_ = {}, --//5
    KB_LASTINPUT = {},
};

-------------------------------------------------
-------------------------------------------------
-------------------------------------------------
function KeySequenceToString(key, bCtrl, bAlt, bShift)
    local keySeqStr = '';
    if bCtrl then
        keySeqStr = keySeqStr .. 'Ctrl + ';
    end
    if bAlt then
        keySeqStr = keySeqStr .. 'Alt + ';
    end
    if bShift then
        keySeqStr = keySeqStr .. 'Shift + ';
    end
    if key == nil or key == '' then
        key = '--'
    end
    keySeqStr = keySeqStr .. key;
    return keySeqStr;
end

-------------------------------------------------
-------------------------------------------------
-- returns a table { HotKey, HotKeyPriority, CtrlDown, AltDown, ShiftDown, HotKeyAlt, HotKeyPriorityAlt, CtrlDownAlt, AltDownAlt, ShiftDownAlt }
function GetHotkey( Cat, Id, bBackup )
    local res = {}
    local name = string.format("'action_%d_%d'", Cat, Id);
    for query in HotkeyManagerData.Query(string.format('SELECT * FROM backup WHERE Name = %s', name)) do
        res.HotKey = query.HotKey;
        res.HotKeyPriority = query.HotKeyPriority;
        res.CtrlDown = query.CtrlDown;
        res.AltDown = query.AltDown;
        res.ShiftDown = query.ShiftDown;
        res.HotKeyAlt = query.HotKeyAlt;
        res.HotKeyPriorityAlt = query.HotKeyPriorityAlt;
        res.CtrlDownAlt = query.CtrlDownAlt;
        res.AltDownAlt = query.AltDownAlt;
        res.ShiftDownAlt = query.ShiftDownAlt;
    end
    if bBackup ~= true then
        for query in HotkeyManagerData.Query(string.format('SELECT * FROM SimpleValues WHERE Name = %s', name)) do
            res.HotKey = query.HotKey;
            res.HotKeyPriority = query.HotKeyPriority;
            res.CtrlDown = query.CtrlDown;
            res.AltDown = query.AltDown;
            res.ShiftDown = query.ShiftDown;
            res.HotKeyAlt = query.HotKeyAlt;
            res.HotKeyPriorityAlt = query.HotKeyPriorityAlt;
            res.CtrlDownAlt = query.CtrlDownAlt;
            res.AltDownAlt = query.AltDownAlt;
            res.ShiftDownAlt = query.ShiftDownAlt;
        end
    end
    return res;
end

-------------------------------------------------
-------------------------------------------------
function SendUpdateHotkey( Cat, Id, HotKey, CtrlDown, AltDown, ShiftDown, HotKeyPriority )
    -- here we 'pack' arguments into single 32-bit integer (kind of binary serialization)
    if (Cat < 0 or Cat > 15 or
        Id < 0 or Id > 65535 or
        HotKeyPriority < 0 or HotKeyPriority > 15) then

        print('WARNING SendUpdateHotkey: arguments out of bounds');
    else
        local product = (2 ^ 28) + (Cat * 2 ^ 24) + (Id * 2 ^ 8) + 0 + ((CtrlDown and 1 or 0) * 2 ^ 6) + ((AltDown and 1 or 0) * 2 ^ 5) + ((ShiftDown and 1 or 0) * 2 ^ 4) + HotKeyPriority;
        -- reason? so we can use Pregame object method which is defined both ingame and in the main menu (unlike 'Game' methods)
        PreGame.SetLeaderKey(product, HotKey);
        Events.GameOptionsChanged.Call()
        LuaEvents.OptMenuHotkeyChanged(Cat, Id, HotKey, CtrlDown, AltDown, ShiftDown)
    end
end

-------------------------------------------------
-------------------------------------------------
-------------------------------------------------
function OnHotkeyEditMode( actionID )
	local action = GameInfoActions[actionID]
	local Cat = action.SubType
	local Id = action.OriginalIndex
    g_InputSeqStr = {'', '', '', ''};
    local HK = GetHotkey(Cat, Id);
    local hotkey = HK.HotKey
    local bCtrl  = HK.CtrlDown and 1 or 0
    local bAlt   = HK.AltDown and 1 or 0
    local bShift = HK.ShiftDown and 1 or 0
    local action = GameInfo[Categories[Cat]][Id]

    Controls.HotkeyDescriptionLabel:LocalizeAndSetText(action.Description or action.Type or '??');
    Controls.HotkeyDescriptionLabel:LocalizeAndSetToolTip(action.Help or Categories[Cat] == 'Builds' and action.Recommendation or '');
    Controls.HotkeyDescriptionIcon:LocalizeAndSetToolTip(action.Help or Categories[Cat] == 'Builds' and action.Recommendation or '');
    Controls.HotkeyDescriptionIcon32:LocalizeAndSetToolTip(action.Help or Categories[Cat] == 'Builds' and action.Recommendation or '');
    -- illustrate actions
    if Categories[Cat] == 'Controls' then
        -- skip
        Controls.HotkeyDescriptionIcon:SetHide(true);
    elseif Categories[Cat] == 'UnitPromotions' or Categories[Cat] == 'Specialists' then
        -- PortraitIndex/IconAtlas
        if action.PortraitIndex ~= -1 then
            if action.IconAtlas == 'ABILITY_ATLAS' then  -- no size 45 icons here :(
                IconHookup( action.PortraitIndex, 32, action.IconAtlas, Controls.HotkeyDescriptionIcon32 );
                Controls.HotkeyDescriptionIcon:SetHide(true);
                Controls.HotkeyDescriptionIcon32:SetHide(false);
            	Controls.HotkeyDescriptionIcon32:SetOffsetVal(160, 0);
            else
                IconHookup( action.PortraitIndex, 45, action.IconAtlas, Controls.HotkeyDescriptionIcon );
            end
        else
            Controls.HotkeyDescriptionIcon:SetHide(true);
        end
    else
        -- IconIndex/IconAtlas
        if action.Type == 'MISSION_FOUND' then  -- special care for city found mission
            Controls.HotkeyDescriptionIcon:SetHide(true);
            Controls.HotkeyDescriptionIcon32:SetHide(false);
            Controls.HotkeyDescriptionIcon32:SetTextureAndResize("Assets/UI/Art/Icons/BuildCity36.dds");
            Controls.HotkeyDescriptionIcon32:SetOffsetVal(158, 0);
        elseif action.IconIndex ~= -1 then
            Controls.HotkeyDescriptionIcon:SetHide(false);
            Controls.HotkeyDescriptionIcon32:SetHide(true);
            IconHookup( action.IconIndex, 45, action.IconAtlas, Controls.HotkeyDescriptionIcon );
        else
            Controls.HotkeyDescriptionIcon:SetHide(true);
        end
    end

    Controls.HotkeyEditButton:SetVoid1(Cat);
    Controls.HotkeyEditButton:SetVoid2(Id);
    -- show possible hotkey collisions
    res = {}
    for row in HotkeyManagerData.Query(
        string.format("SELECT * FROM (\
            SELECT * FROM backup WHERE Name NOT IN (SELECT Name FROM SimpleValues)\
            UNION ALL\
            SELECT Name, HotKey, AltDownAlt, ShiftDownAlt, ActionIndex, CtrlDown, HotKeyPriority, ActionSubType, HotKeyAlt, HotKeyPriorityAlt, CtrlDownAlt, ShiftDown, AltDown FROM SimpleValues\
            WHERE Name LIKE 'action\\_%%' ESCAPE '\\'\
            )\
            WHERE Name <> '%s' AND (HotKey = '%s' AND CtrlDown = %s AND AltDown = %s AND ShiftDown = %s)\
            ORDER BY Name", string.format('action_%d_%d', Cat, Id), hotkey, bCtrl, bAlt, bShift))
    do
    	local __cat = row.ActionSubType
    	local __id = row.ActionIndex
    	local __action = GameInfo[Categories[__cat]][__id]
        res[#res+1] = string.format('[ICON_BULLET]%s', __action.Description and Locale.Lookup(__action.Description) or '--')
    end
    if #res > 0 then
        table.sort(res, function(a,b)
            return Locale.Compare(a, b) < 0
        end)
        ttbox.Text:LocalizeAndSetText(string.format('{TXT_KEY_OPSCREEN_HOTKEY_CONFLICT}[NEWLINE]%s', table.concat(res, '[NEWLINE]')))
        ttbox.Root:SetHide(false)
        ttbox.Root:DoAutoSize()
    else
        ttbox.Root:SetHide(true)
    end
    Controls.HotkeyEditButton:SetToolTipCallback(function(con)
    	if #res > 0 then
    	    table.sort(res, function(a,b)
    	        return Locale.Compare(a, b) < 0
    	    end)
    	    ttbox.Text:LocalizeAndSetText(string.format('{TXT_KEY_OPSCREEN_HOTKEY_CONFLICT}[NEWLINE]%s', table.concat(res, '[NEWLINE]')))
    	    ttbox.Root:SetHide(false)
    	    ttbox.Root:DoAutoSize()
    	else
    	    ttbox.Root:SetHide(true)
    	end
    end);

    Controls.HotkeyEditFrontLabel:SetText(KeySequenceToString(g_KeyMap[HK.HotKey] and g_KeyMap[HK.HotKey][3] or '--', HK.CtrlDown, HK.AltDown, HK.ShiftDown));

    Controls.HotkeyEditCancelButton:SetVoid1(Cat);
    Controls.HotkeyEditCancelButton:SetVoid2(Id);
    Controls.HotkeyEditCancelButton:RegisterCallback(Mouse.eLClick, OnCancelEditClick);

    Controls.HotkeyEditConfirmButton:SetVoid1(Cat);
    Controls.HotkeyEditConfirmButton:SetVoid2(Id);
    Controls.HotkeyEditConfirmButton:RegisterCallback(Mouse.eLClick, OnConfirmEditClick);

    Controls.HotkeyEditResetButton:SetVoid1(Cat);
    Controls.HotkeyEditResetButton:SetVoid2(Id);
    Controls.HotkeyEditResetButton:RegisterCallback(Mouse.eLClick, OnResetHotkeyClick);

    Controls.HotkeyEditFrontLabel:LocalizeAndSetText('TXT_KEY_OPSCREEN_EDIT_HOTKEY_MODE');
    Controls.HotkeyEditButtonsBox:SetHide(false);
    Controls.HotkeyOuterBox:SetColorVal(231/255, 213/255, 0/255, 255/255);  -- COLOR_YIELD_GOLD

    g_ListenInput = true;
end

-------------------------------------------------
-------------------------------------------------
function OnConfirmEditClick( Cat, Id )
    g_ListenInput = false;

    local bCtrl = g_InputSeqStr[1] ~= '';
    local bAlt = g_InputSeqStr[2] ~= '';
    local bShift = g_InputSeqStr[3] ~= '';
    local key = g_InputSeqStr[4] or '';

    SendUpdateHotkey(Cat, Id, key, bCtrl, bAlt, bShift, 0);
    local DBstr = 'action_' .. Cat .. '_' .. Id;
    local q = string.format("REPLACE INTO SimpleValues (Name, ActionSubType, ActionIndex, HotKey, CtrlDown, AltDown, ShiftDown) VALUES ('%s', %d, %d, '%s', %d, %d, %d)", DBstr, Cat, Id, g_InputSeqStr[4], bCtrl and 1 or 0, bAlt and 1 or 0, bShift and 1 or 0)
    for row in HotkeyManagerData.Query(q) do
    end;

    OnBack()
end

-------------------------------------------------
-------------------------------------------------
function OnCancelEditClick( Cat, Id )
    g_ListenInput = false;

    OnBack()
end

-------------------------------------------------
-------------------------------------------------
function OnResetHotkeyClick( Cat, Id )
    g_ListenInput = false;
    -- delete userdata entry and fallback to original hotkey
    for row in HotkeyManagerData.Query(string.format("DELETE FROM SimpleValues WHERE Name = 'action_%d_%d'", Cat, Id)) do
    end
    local HK = GetHotkey(Cat, Id, true);
    SendUpdateHotkey(Cat, Id, HK.HotKey, HK.CtrlDown, HK.AltDown, HK.ShiftDown, 0);

    OnBack()
end

-------------------------------------------------
-------------------------------------------------
-- on script load, make sure all columns are presented
-- in user data DB. if not, recreate the missing ones
function VerifyUserDataIntegrity()
    local columns = {};
    local newColumns = {
        ActionSubType = { 'integer', 0 },
        ActionIndex = { 'integer', 0 },
        HotKey = { 'text', '' },
        HotKeyPriority = { 'integer', 0 },
        CtrlDown = { 'boolean', 0 },
        AltDown = { 'boolean', 0 },
        ShiftDown = { 'boolean', 0 },
        HotKeyAlt = { 'text', '' },
        HotKeyPriorityAlt = { 'integer', 0 },
        CtrlDownAlt = { 'boolean', 0 },
        AltDownAlt = { 'boolean', 0 },
        ShiftDownAlt = { 'boolean', 0 },
    }
    for row in HotkeyManagerData.Query([[PRAGMA table_info(SimpleValues)]]) do
        columns[row.name] = true;
    end
    for col, info in next, newColumns do
        if columns[col] == nil then
            local defaultStr = '';
            if info[2] ~= '' then
                defaultStr = ' DEFAULT ' .. info[2];
            end
            for row in HotkeyManagerData.Query(string.format('ALTER TABLE SimpleValues ADD %s %s%s', col, info[1], defaultStr)) do
            end
        end
    end
end

-------------------------------------------------
-------------------------------------------------

ttbox = {}
TTManager:GetTypeControlTable( "Simple", ttbox )

----------------------------------------------------------------
-- Key Down Processing
----------------------------------------------------------------
function InputHandler( uiMsg, wParam, lParam )
    --print(uiMsg, wParam)
    if( g_ListenInput == true ) then
        local keySeqStr = ''
        if uiMsg == KeyEvents.KeyDown or uiMsg == 260 then
            if UIManager:GetControl() then
                g_InputSeqStr[1] = 'Ctrl + '
            else
                g_InputSeqStr[1] = ''
            end
            if UIManager:GetAlt() then
                g_InputSeqStr[2] = 'Alt + ';
            else
                g_InputSeqStr[2] = ''
            end
            if UIManager:GetShift() then
                g_InputSeqStr[3] = 'Shift + ';
            else
                g_InputSeqStr[3] = ''
            end
            if wParam ~= 16 and wParam ~= 17 and wParam ~= 18 then
                for k,v in pairs(g_KeyMap) do
                    if v[1] == wParam then
                        g_InputSeqStr[4] = k;
                    end
                end
            else
                g_InputSeqStr[4] = (g_InputSeqStr[4] ~= '' and g_InputSeqStr[4]) or '...';
            end

            Controls.HotkeyEditFrontLabel:SetText(g_InputSeqStr[1] .. g_InputSeqStr[2] .. g_InputSeqStr[3] .. (g_InputSeqStr[4] ~= '...' and g_KeyMap[g_InputSeqStr[4]] and g_KeyMap[g_InputSeqStr[4]][3] or '...'));
            -- show possible hotkey collisions (update tt)
    		local Cat = Controls.HotkeyEditButton:GetVoid1()
    		local Id = Controls.HotkeyEditButton:GetVoid2()
        	res = {}
        	for row in HotkeyManagerData.Query(
        	    string.format("SELECT * FROM (\
        	        SELECT * FROM backup WHERE Name NOT IN (SELECT Name FROM SimpleValues)\
        	        UNION ALL\
        	        SELECT Name, HotKey, AltDownAlt, ShiftDownAlt, ActionIndex, CtrlDown, HotKeyPriority, ActionSubType, HotKeyAlt, HotKeyPriorityAlt, CtrlDownAlt, ShiftDown, AltDown FROM SimpleValues\
        	        WHERE Name LIKE 'action\\_%%' ESCAPE '\\'\
        	        )\
        	        WHERE Name <> '%s' AND (HotKey = '%s' AND CtrlDown = %s AND AltDown = %s AND ShiftDown = %s)\
        	        ORDER BY Name", string.format('action_%d_%d', Cat, Id), g_InputSeqStr[4], UIManager:GetControl() and 1 or 0, UIManager:GetAlt() and 1 or 0, UIManager:GetShift() and 1 or 0))
        	do
        		local __cat = row.ActionSubType
        		local __id = row.ActionIndex
        		local __action = GameInfo[Categories[__cat]][__id]
        	    res[#res+1] = string.format('[ICON_BULLET]%s', __action.Description and Locale.Lookup(__action.Description) or '--')
        	end
            if #res > 0 then
                table.sort(res, function(a,b)
                    return Locale.Compare(a, b) < 0
                end)
                ttbox.Text:LocalizeAndSetText(string.format('{TXT_KEY_OPSCREEN_HOTKEY_CONFLICT}[NEWLINE]%s', table.concat(res, '[NEWLINE]')))
                ttbox.Root:SetHide(false)
                ttbox.Root:DoAutoSize()
            else
                ttbox.Root:SetHide(true)
            end
        end
    else
    	return false  -- continue input propagation
    end
    return true  -- eat input
end
ContextPtr:SetInputHandler( InputHandler );

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
function ShowHideHandler( bIsHide, bIsInit )

    if( not bIsInit ) then
        if( not bIsHide ) then
	        UI.incTurnTimerSemaphore();
        else
	        UI.decTurnTimerSemaphore();
        end
    else
        VerifyUserDataIntegrity();
    end

end
ContextPtr:SetShowHideHandler( ShowHideHandler );

function OnPopup( actionID )
    OnHotkeyEditMode( actionID )
    UIManager:QueuePopup( ContextPtr, PopupPriority.eUtmost );
end
LuaEvents.QuickHotkeyEdit.Add( OnPopup );

----------------------------------------------------------------
----------------------------------------------------------------
function OnBack()
	UIManager:DequeuePopup( ContextPtr );
end

----------------------------------------------------------------
-- 'Active' (local human) player has changed
----------------------------------------------------------------
Events.GameplaySetActivePlayer.Add(OnBack);
