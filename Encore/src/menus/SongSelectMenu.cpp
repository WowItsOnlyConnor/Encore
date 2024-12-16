//
// Created by marie on 16/11/2024.
//

#include "raylib.h"
#include "SongSelectMenu.h"


#include "MenuManager.h"
#include "gameMenu.h"
#include "raygui.h"
#include "uiUnits.h"
#include "gameplay/gameplayRenderer.h"
#include "song/songlist.h"

int songSelectOffset = 0;
SortType currentSortValue = SortType::Title;
Color AccentColor = {255,0,255,255};

void SongSelectMenu::Load() {
    TheSongList.curSong->LoadAlbumArt();
    SetTextureWrap(TheSongList.curSong->albumArtBlur, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(TheSongList.curSong->albumArtBlur, TEXTURE_FILTER_ANISOTROPIC_16X);
    songSelectOffset = TheSongList.curSong->songListPos - 5;
    TheGameRenderer.streamsLoaded = false;
    TheGameRenderer.midiLoaded = false;
}

void SongSelectMenu::Draw() {
    Assets &assets = Assets::getInstance();
    Units u = Units::getInstance();
    // TheGameRenderer.songEnded = false;
    //  player.overdrive = false;
    // TheGameRenderer.curNoteIdx = {0, 0, 0, 0, 0};
    // TheGameRenderer.curODPhrase = 0;
    // TheGameRenderer.curNoteInt = 0;
    // TheGameRenderer.curSolo = 0;
    // TheGameRenderer.curBeatLine = 0;
    // TheGameRenderer.curBPM = 0;

    // if (selSong)
    // TheGameRenderer.selectedSongInt = curPlayingSong;
    // else
    // TheGameRenderer.selectedSongInt = menu.ChosenSongInt;

    double curTime = GetTime();
    // -5 -4 -3 -2 -1 0 1 2 3 4 5 6
    Vector2 mouseWheel = GetMouseWheelMoveV();
    int lastIntChosen = (int)mouseWheel.y;
    // set to specified height
    if (songSelectOffset <= TheSongList.listMenuEntries.size() && songSelectOffset >= 1
        && TheSongList.listMenuEntries.size() >= 10) {
        songSelectOffset -= (int)mouseWheel.y;
    }

    // prevent going past top
    if (songSelectOffset < 1)
        songSelectOffset = 1;

    // prevent going past bottom
    if (songSelectOffset >= TheSongList.listMenuEntries.size() - 10)
        songSelectOffset = TheSongList.listMenuEntries.size() - 10;

    // todo(3drosalia): clean this shit up after changing it

    Song SongToDisplayInfo;
    BeginShaderMode(assets.bgShader);
    // todo(3drosalia): this too
    SongToDisplayInfo = *TheSongList.curSong;
    if (TheSongList.curSong->ini)
        TheSongList.curSong->LoadInfoINI(TheSongList.curSong->songInfoPath);
    else
        TheSongList.curSong->LoadInfo(TheSongList.curSong->songInfoPath);
    GameMenu::DrawAlbumArtBackground(TheSongList.curSong->albumArtBlur);
    EndShaderMode();

    float TopOvershell = u.hpct(0.15f);
    DrawRectangle(
        0, 0, u.RightSide - u.LeftSide, (float)GetScreenHeight(), GetColor(0x00000080)
    );
    // DrawLineEx({u.LeftSide + u.winpct(0.0025f), 0}, {
    // 				u.LeftSide + u.winpct(0.0025f), (float) GetScreenHeight()
    // 			}, u.winpct(0.005f), WHITE);
    BeginScissorMode(0, u.hpct(0.15f), u.RightSide - u.winpct(0.25f), u.hinpct(0.7f));
    GameMenu::DrawTopOvershell(0.208333f);
    EndScissorMode();
    encOS::DrawTopOvershell(0.15f);

    GameMenu::DrawVersion();
    int AlbumX = u.RightSide - u.winpct(0.25f);
    int AlbumY = u.hpct(0.075f);
    int AlbumHeight = u.winpct(0.25f);
    int AlbumOuter = u.hinpct(0.01f);
    int AlbumInner = u.hinpct(0.005f);
    int BorderBetweenAlbumStuff = (u.RightSide - u.LeftSide) - u.winpct(0.25f);

    DrawTextEx(
        assets.josefinSansItalic,
        TextFormat("Sorted by: %s", sortTypes[(int)currentSortValue].c_str()),
        { u.LeftSide, u.hinpct(0.165f) },
        u.hinpct(0.03f),
        0,
        WHITE
    );
    DrawTextEx(
        assets.josefinSansItalic,
        TextFormat("Songs loaded: %01i", TheSongList.songs.size()),
        { AlbumX - (AlbumOuter * 2)
              - MeasureTextEx(
                    assets.josefinSansItalic,
                    TextFormat("Songs loaded: %01i", TheSongList.songs.size()),
                    u.hinpct(0.03f),
                    0
              )
                    .x,
          u.hinpct(0.165f) },
        u.hinpct(0.03f),
        0,
        WHITE
    );

    float songEntryHeight = u.hinpct(0.058333f);
    DrawRectangle(
        0,
        u.hinpct(0.208333f),
        (u.RightSide - u.winpct(0.25f)),
        songEntryHeight,
        ColorBrightness(AccentColor, -0.75f)
    );

    for (int j = 0; j < 5; j++) {
        DrawRectangle(
            0,
            ((songEntryHeight * 2) * j) + u.hinpct(0.208333f) + songEntryHeight,
            (u.RightSide - u.winpct(0.25f)),
            songEntryHeight,
            Color { 0, 0, 0, 64 }
        );
    }

    for (int i = songSelectOffset;
         i < TheSongList.listMenuEntries.size() && i < songSelectOffset + 10;
         i++) {
        if (TheSongList.listMenuEntries.size() == i)
            break;
        if (TheSongList.listMenuEntries[i].isHeader) {
            float songXPos = u.LeftSide + u.winpct(0.005f) - 2;
            float songYPos = std::floor(
                (u.hpct(0.266666f)) + ((songEntryHeight) * ((i - songSelectOffset)))
            );
            DrawRectangle(
                0,
                songYPos,
                (u.RightSide - u.winpct(0.25f)),
                songEntryHeight,
                ColorBrightness(AccentColor, -0.75f)
            );

            DrawTextEx(
                assets.rubikBold,
                TheSongList.listMenuEntries[i].headerChar.c_str(),
                { songXPos, songYPos + u.hinpct(0.0125f) },
                u.hinpct(0.035f),
                0,
                WHITE
            );
        } else if (!TheSongList.listMenuEntries[i].hiddenEntry) {

            bool isCurSong = i == TheSongList.curSong->songListPos - 1;
            Font &artistFont =
                isCurSong ? assets.josefinSansItalic : assets.josefinSansItalic;
            Song &songi = TheSongList.songs[TheSongList.listMenuEntries[i].songListID];
            int songID = TheSongList.listMenuEntries[i].songListID;


            // float buttonX =
            // ((float)GetScreenWidth()/2)-(((float)GetScreenWidth()*0.86f)/2);
            // LerpState state = lerpCtrl.createLerp("SONGSELECT_LERP_" +
            // std::to_string(i), EaseOutCirc, 0.4f);
            float songXPos = u.LeftSide + u.winpct(0.005f) - 2;
            float songYPos = std::floor(
                (u.hpct(0.266666f)) + ((songEntryHeight) * ((i - songSelectOffset)))
            );
            GuiSetStyle(BUTTON, BORDER_WIDTH, 0);
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0);
            if (isCurSong) {
                GuiSetStyle(
                    BUTTON,
                    BASE_COLOR_NORMAL,
                    ColorToInt(ColorBrightness(AccentColor, -0.4))
                );
            }
            BeginScissorMode(
                0, u.hpct(0.15f), u.RightSide - u.winpct(0.25f), u.hinpct(0.7f)
            );
            if (GuiButton(
                    Rectangle {
                        0, songYPos, (u.RightSide - u.winpct(0.25f)), songEntryHeight },
                    ""
                )) {
                // curPlayingSong = songID;
                // selSong = true;
                // albumArtLoaded = false;
                TheSongList.curSong = &TheSongList.songs[songID];
                if (!TheSongList.songs[songID].AlbumArtLoaded) {
                    try {
                        TheSongList.songs[songID].LoadAlbumArt();
                        TheSongList.songs[songID].AlbumArtLoaded = true;
                    } catch (const std::exception &e) {
                        Encore::EncoreLog(LOG_ERROR, e.what());
                    }
                }
            }
            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x181827FF);
            EndScissorMode();
            // DrawTexturePro(song.albumArt, Rectangle{
            // songXPos,0,(float)song.albumArt.width,(float)song.albumArt.height
            // }, { songXPos+5,songYPos + 5,50,50 }, Vector2{ 0,0 }, 0.0f,
            // RAYWHITE);
            int songTitleWidth = (u.winpct(0.3f)) - 6;

            int songArtistWidth = (u.winpct(0.5f)) - 6;

            if (songi.titleTextWidth >= (float)songTitleWidth) {
                if (curTime > songi.titleScrollTime
                    && curTime < songi.titleScrollTime + 3.0)
                    songi.titleXOffset = 0;

                if (curTime > songi.titleScrollTime + 3.0) {
                    songi.titleXOffset -= 1;

                    if (songi.titleXOffset
                        < -(songi.titleTextWidth - (float)songTitleWidth)) {
                        songi.titleXOffset =
                            -(songi.titleTextWidth - (float)songTitleWidth);
                        songi.titleScrollTime = curTime + 3.0;
                    }
                }
            }
            auto LightText = Color { 203, 203, 203, 255 };
            BeginScissorMode(
                (int)songXPos + (isCurSong ? 5 : 20),
                (int)songYPos,
                songTitleWidth,
                songEntryHeight
            );
            DrawTextEx(
                assets.rubikBold,
                songi.title.c_str(),
                { songXPos + songi.titleXOffset + (isCurSong ? 10 : 20),
                  songYPos + u.hinpct(0.0125f) },
                u.hinpct(0.035f),
                0,
                isCurSong ? WHITE : LightText
            );
            EndScissorMode();

            if (songi.artistTextWidth > (float)songArtistWidth) {
                if (curTime > songi.artistScrollTime
                    && curTime < songi.artistScrollTime + 3.0)
                    songi.artistXOffset = 0;

                if (curTime > songi.artistScrollTime + 3.0) {
                    songi.artistXOffset -= 1;
                    if (songi.artistXOffset
                        < -(songi.artistTextWidth - (float)songArtistWidth)) {
                        songi.artistScrollTime = curTime + 3.0;
                    }
                }
            }

            auto SelectedText = WHITE;
            BeginScissorMode(
                (int)songXPos + 30 + (int)songTitleWidth,
                (int)songYPos,
                songArtistWidth,
                songEntryHeight
            );
            DrawTextEx(
                artistFont,
                songi.artist.c_str(),
                { songXPos + 30 + (float)songTitleWidth + songi.artistXOffset,
                  songYPos + u.hinpct(0.02f) },
                u.hinpct(0.025f),
                0,
                isCurSong ? WHITE : LightText
            );
            EndScissorMode();
        }
    }

    DrawRectangle(
        AlbumX - AlbumOuter,
        AlbumY + AlbumHeight,
        AlbumHeight + AlbumOuter,
        AlbumHeight + u.hinpct(0.01f),
        WHITE
    );
    DrawRectangle(
        AlbumX - AlbumInner,
        AlbumY + AlbumHeight,
        AlbumHeight,
        u.hinpct(0.075f) + AlbumHeight,
        GetColor(0x181827FF)
    );
    DrawRectangle(
        AlbumX - AlbumOuter,
        AlbumY - AlbumInner,
        AlbumHeight + AlbumOuter,
        AlbumHeight + AlbumOuter,
        WHITE
    );
    DrawRectangle(AlbumX - AlbumInner, AlbumY, AlbumHeight, AlbumHeight, BLACK);
    // TODO: replace this with actual sorting/category hiding
    if (songSelectOffset > 0) {
        std::string SongTitleForCharThingyThatsTemporary =
            TheSongList.listMenuEntries[songSelectOffset].headerChar;
        switch (currentSortValue) {
        case SortType::Title: {
            if (TheSongList.listMenuEntries[songSelectOffset].isHeader) {
                SongTitleForCharThingyThatsTemporary =
                    TheSongList
                        .songs[TheSongList.listMenuEntries[songSelectOffset - 1].songListID]
                        .title[0];
            } else {
                SongTitleForCharThingyThatsTemporary =
                    TheSongList
                        .songs[TheSongList.listMenuEntries[songSelectOffset].songListID]
                        .title[0];
            }
            break;
        }
        case SortType::Artist: {
            if (TheSongList.listMenuEntries[songSelectOffset].isHeader) {
                SongTitleForCharThingyThatsTemporary =
                    TheSongList
                        .songs[TheSongList.listMenuEntries[songSelectOffset - 1].songListID]
                        .artist[0];
            } else {
                SongTitleForCharThingyThatsTemporary =
                    TheSongList
                        .songs[TheSongList.listMenuEntries[songSelectOffset].songListID]
                        .artist[0];
            }
            break;
        }
        case SortType::Source: {
            if (TheSongList.listMenuEntries[songSelectOffset].isHeader) {
                SongTitleForCharThingyThatsTemporary =
                    TheSongList
                        .songs[TheSongList.listMenuEntries[songSelectOffset - 1].songListID]
                        .source;
            } else {
                SongTitleForCharThingyThatsTemporary =
                    TheSongList
                        .songs[TheSongList.listMenuEntries[songSelectOffset].songListID]
                        .source;
            }
            break;
        }
        case SortType::Length: {
            if (TheSongList.listMenuEntries[songSelectOffset].isHeader) {
                SongTitleForCharThingyThatsTemporary = std::to_string(
                    TheSongList
                        .songs[TheSongList.listMenuEntries[songSelectOffset - 1].songListID]
                        .length
                );
            } else {
                SongTitleForCharThingyThatsTemporary = std::to_string(
                    TheSongList
                        .songs[TheSongList.listMenuEntries[songSelectOffset].songListID]
                        .length
                );
            }
            break;
        }
        }

        DrawTextEx(
            assets.rubikBold,
            SongTitleForCharThingyThatsTemporary.c_str(),
            { u.LeftSide + 5, u.hpct(0.218333f) },
            u.hinpct(0.035f),
            0,
            WHITE
        );
    }

    // bottom
    DrawTexturePro(
        TheSongList.curSong->albumArt,
        Rectangle { 0,
                    0,
                    (float)TheSongList.curSong->albumArt.width,
                    (float)TheSongList.curSong->albumArt.width },
        Rectangle { (float)AlbumX - AlbumInner,
                    (float)AlbumY,
                    (float)AlbumHeight,
                    (float)AlbumHeight },
        { 0, 0 },
        0,
        WHITE
    );
    // hehe

    float TextPlacementTB = u.hpct(0.05f);
    float TextPlacementLR = u.LeftSide;
    GameMenu::mhDrawText(
        assets.redHatDisplayBlack,
        "MUSIC LIBRARY",
        { TextPlacementLR, TextPlacementTB },
        u.hinpct(0.125f),
        WHITE,
        assets.sdfShader,
        LEFT
    );

    std::string AlbumArtText =
        SongToDisplayInfo.album.empty() ? "No Album Listed" : SongToDisplayInfo.album;

    float AlbumTextHeight =
        MeasureTextEx(assets.rubikBold, AlbumArtText.c_str(), u.hinpct(0.035f), 0).y;
    float AlbumTextWidth =
        MeasureTextEx(assets.rubikBold, AlbumArtText.c_str(), u.hinpct(0.035f), 0).x;
    float AlbumNameTextCenter = u.RightSide - u.winpct(0.125f) - AlbumInner;
    float AlbumTTop = AlbumY + AlbumHeight + u.hinpct(0.011f);
    float AlbumNameFontSize = AlbumTextWidth <= u.winpct(0.25f)
        ? u.hinpct(0.035f)
        : u.winpct(0.23f) / (AlbumTextWidth / AlbumTextHeight);
    float AlbumNameLeft = AlbumNameTextCenter
        - (MeasureTextEx(assets.rubikBold, AlbumArtText.c_str(), AlbumNameFontSize, 0).x
           / 2);
    float AlbumNameTextTop = AlbumTextWidth <= u.winpct(0.25f)
        ? AlbumTTop
        : AlbumTTop + ((u.hinpct(0.035f) / 2) - (AlbumNameFontSize / 2));

    DrawTextEx(
        assets.rubikBold,
        AlbumArtText.c_str(),
        { AlbumNameLeft, AlbumNameTextTop },
        AlbumNameFontSize,
        0,
        WHITE
    );

    DrawLine(
        u.RightSide - AlbumHeight - AlbumOuter,
        AlbumY + AlbumHeight + AlbumOuter + (u.hinpct(0.04f)),
        u.RightSide,
        AlbumY + AlbumHeight + AlbumOuter + (u.hinpct(0.04f)),
        WHITE
    );

    float DiffTop = AlbumY + AlbumHeight + AlbumOuter + (u.hinpct(0.045f));

    float DiffHeight = u.hinpct(0.035f);
    float DiffTextSize = u.hinpct(0.03f);
    float DiffDotLeft =
        u.RightSide - MeasureTextEx(assets.rubikBold, "OOOOO  ", DiffHeight, 0).x;
    float ScrollbarLeft = u.RightSide - AlbumHeight - AlbumOuter - (AlbumInner * 2);
    float ScrollbarTop = u.hpct(0.208333f);
    float ScrollbarHeight = GetScreenHeight() - u.hpct(0.208333f) - u.hpct(0.15f);
    GuiSetStyle(SCROLLBAR, BACKGROUND_COLOR, 0x181827FF);
    GuiSetStyle(SCROLLBAR, SCROLL_SLIDER_SIZE, u.hinpct(0.03f));
    /*
    songSelectOffset = GuiScrollBar(
        { ScrollbarLeft, ScrollbarTop, (float)(AlbumInner * 2), ScrollbarHeight },
        songSelectOffset,
        1,
        TheSongList.listMenuEntries.size() - 10
    );
    */
    float IconWidth = float(AlbumHeight - AlbumOuter) / 5.0f;
    GameMenu::mhDrawText(
        assets.rubikItalic,
        "Pad",
        { (u.RightSide - AlbumHeight + AlbumInner), DiffTop },
        AlbumOuter * 3,
        WHITE,
        assets.sdfShader,
        LEFT
    );
    GameMenu::mhDrawText(
        assets.rubikItalic,
        "Classic",
        { (u.RightSide - AlbumHeight + AlbumInner),
          DiffTop + IconWidth + (AlbumOuter * 3) },
        AlbumOuter * 3,
        WHITE,
        assets.sdfShader,
        LEFT
    );
    for (int i = 0; i < 10; i++) {
        bool RowTwo = i < 5;
        int RowTwoInt = i - 5;
        float PosTopAddition = RowTwo ? AlbumOuter * 3 : AlbumOuter * 6;
        float BoxTopPos = DiffTop + PosTopAddition + float(IconWidth * (RowTwo ? 0 : 1));
        float ResetToLeftPos = (float)(RowTwo ? i : RowTwoInt);
        int asdasd = (float)(RowTwo ? i : RowTwoInt);
        float IconLeftPos =
            (float)(u.RightSide - AlbumHeight) + IconWidth * ResetToLeftPos;
        Rectangle Placement = { IconLeftPos, BoxTopPos, IconWidth, IconWidth };
        Color TintColor = WHITE;
        if (SongToDisplayInfo.parts[i]->diff == -1)
            TintColor = DARKGRAY;
        DrawTexturePro(
            assets.InstIcons[asdasd],
            { 0,
              0,
              (float)assets.InstIcons[asdasd].width,
              (float)assets.InstIcons[asdasd].height },
            Placement,
            { 0, 0 },
            0,
            TintColor
        );
        DrawTexturePro(
            assets.BaseRingTexture,
            { 0,
              0,
              (float)assets.BaseRingTexture.width,
              (float)assets.BaseRingTexture.height },
            Placement,
            { 0, 0 },
            0,
            ColorBrightness(WHITE, 2)
        );
        if (SongToDisplayInfo.parts[i]->diff > 0)
            DrawTexturePro(
                assets.YargRings[SongToDisplayInfo.parts[i]->diff - 1],
                { 0,
                  0,
                  (float)assets.YargRings[SongToDisplayInfo.parts[i]->diff - 1].width,
                  (float)assets.YargRings[SongToDisplayInfo.parts[i]->diff - 1].height },
                Placement,
                { 0, 0 },
                0,
                WHITE
            );
    }

    GameMenu::DrawBottomOvershell();
    float BottomOvershell = (float)GetScreenHeight() - 120;

    GuiSetStyle(
        BUTTON, BASE_COLOR_NORMAL, ColorToInt(ColorBrightness(AccentColor, -0.25))
    );
    if (GuiButton(
            Rectangle { u.LeftSide,
                        GetScreenHeight() - u.hpct(0.1475f),
                        u.winpct(0.2f),
                        u.hinpct(0.05f) },
            "Play Song"
        )) {
        // curPlayingSong = menu.ChosenSongInt;
        if (!TheSongList.curSong->ini) {
            TheSongList.curSong->LoadSong(TheSongList.curSong->songInfoPath);
        } else {
            TheSongList.curSong->LoadSongIni(TheSongList.curSong->songDir);
        }
        TheMenuManager.SwitchScreen(READY_UP);
    }
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x181827FF);

    if (GuiButton(
            Rectangle { u.LeftSide + u.winpct(0.4f) - 2,
                        GetScreenHeight() - u.hpct(0.1475f),
                        u.winpct(0.2f),
                        u.hinpct(0.05f) },
            "Sort"
        )) {
        currentSortValue = NextSortType(currentSortValue);
        TheSongList.sortList(currentSortValue, TheSongList.curSong->songListPos);
    }
    if (GuiButton(
            Rectangle { u.LeftSide + u.winpct(0.2f) - 1,
                        GetScreenHeight() - u.hpct(0.1475f),
                        u.winpct(0.2f),
                        u.hinpct(0.05f) },
            "Back"
        )) {
        for (Song &songi : TheSongList.songs) {
            songi.titleXOffset = 0;
            songi.artistXOffset = 0;
        }
        TheMenuManager.SwitchScreen(MAIN_MENU);
    }
    DrawOvershell();
}