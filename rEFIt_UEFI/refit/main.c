/*
 * refit/main.c
 * Main code for the boot menu
 *
 * Copyright (c) 2006-2010 Christoph Pfisterer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of Christoph Pfisterer nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Platform.h"
//#include "../include/Handle.h"

#include "Version.h"

#ifndef DEBUG_ALL
#define DEBUG_MAIN 1
#else
#define DEBUG_MAIN DEBUG_ALL
#endif

#if DEBUG_MAIN == 0
#define DBG(...)
#else
#define DBG(...) DebugLog(DEBUG_MAIN, __VA_ARGS__)
#endif

#ifndef DEBUG_ALL
#define DEBUG_TIME 1
#else
#define DEBUG_TIME DEBUG_ALL
#endif

#if DEBUG_TIME == 0
  #define DBGT(...)
#else
  #define DBGT(...) { DBG("[%a] ", DbgTime()); DBG(__VA_ARGS__); }
#endif

// variables

#define MACOSX_LOADER_PATH      L"\\System\\Library\\CoreServices\\boot.efi"

//static CHAR8 FirmwareRevisionStr[] = FIRMWARE_REVISION_STR;

EFI_HANDLE              gImageHandle;
EFI_SYSTEM_TABLE*       gST;
EFI_BOOT_SERVICES*		  gBS; 
EFI_RUNTIME_SERVICES*	  gRS;
EFI_DXE_SERVICES*       gDS;

static REFIT_MENU_ENTRY MenuEntryOptions  = { L"Options", TAG_OPTIONS, 1, 0, 'O', NULL, NULL, {0, 0, 0, 0}, ActionEnter, ActionEnter, ActionNone, NULL };
static REFIT_MENU_ENTRY MenuEntryAbout    = { L"About rEFIt", TAG_ABOUT, 1, 0, 'A', NULL, NULL, {0, 0, 0, 0}, ActionEnter, ActionEnter, ActionNone,  NULL };
static REFIT_MENU_ENTRY MenuEntryReset    = { L"Restart Computer", TAG_RESET, 1, 0, 'R', NULL, NULL, {0, 0, 0, 0}, ActionSelect, ActionEnter, ActionNone,  NULL };
static REFIT_MENU_ENTRY MenuEntryShutdown = { L"Shut Down Computer", TAG_SHUTDOWN, 1, 0, 'U', NULL, NULL, {0, 0, 0, 0}, ActionSelect, ActionEnter, ActionNone,  NULL };
REFIT_MENU_ENTRY MenuEntryReturn   = { L"Return to Main Menu", TAG_RETURN, 0, 0, 0, NULL, NULL, {0, 0, 0, 0}, ActionEnter, ActionEnter, ActionNone,  NULL };

static REFIT_MENU_SCREEN MainMenu    = {1, L"Main Menu", NULL, 0, NULL, 0, NULL, 0, L"Automatic boot", FALSE, FALSE, 0, 0, 0,
  0, {0, 0, 0, 0}, NULL};
static REFIT_MENU_SCREEN AboutMenu   = {2, L"About", NULL, 0, NULL, 0, NULL, 0, NULL, FALSE, FALSE, 0, 0, 0, 0, {0, 0, 0, 0}, NULL };
static REFIT_MENU_SCREEN HelpMenu    = {3, L"Help",  NULL, 0, NULL, 0, NULL, 0, NULL, FALSE, FALSE, 0, 0, 0, 0, {0, 0, 0, 0}, NULL };

DRIVERS_FLAGS gDriversFlags = {FALSE, FALSE, FALSE};

static VOID AboutRefit(VOID)
{
//  CHAR8* Revision = NULL;
    if (AboutMenu.EntryCount == 0) {
        AboutMenu.TitleImage = BuiltinIcon(BUILTIN_ICON_FUNC_ABOUT);
        AddMenuInfoLine(&AboutMenu, L"rEFIt Version 2.01 UEFI by Slice");
#ifdef FIRMWARE_BUILDDATE
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" Build: %a", FIRMWARE_BUILDDATE));
#else
        AddMenuInfoLine(&AboutMenu, L" Build: unknown");
#endif
        AddMenuInfoLine(&AboutMenu, L"");
        AddMenuInfoLine(&AboutMenu, L"Copyright (c) 2006-2010 Christoph Pfisterer");
        AddMenuInfoLine(&AboutMenu, L"Portions Copyright (c) Intel Corporation and others");
        AddMenuInfoLine(&AboutMenu, L"");
        AddMenuInfoLine(&AboutMenu, L"Running on:");
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" EFI Revision %d.%02d",
            gST->Hdr.Revision >> 16, gST->Hdr.Revision & ((1 << 16) - 1)));
#if defined(MDE_CPU_IA32)
        AddMenuInfoLine(&AboutMenu, L" Platform: i386 (32 bit)");
#elif defined(MDE_CPU_X64)
        AddMenuInfoLine(&AboutMenu, L" Platform: x86_64 (64 bit)");
#else
        AddMenuInfoLine(&AboutMenu, L" Platform: unknown");
#endif
#ifdef FIRMWARE_REVISION
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" Firmware: %s rev %s", gST->FirmwareVendor, FIRMWARE_REVISION));
#else
      AddMenuInfoLine(&AboutMenu, PoolPrint(L" Firmware: %s rev %d", gST->FirmwareVendor, gST->FirmwareRevision));
#endif
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" Screen Output: %s", egScreenDescription()));
      AboutMenu.AnimeRun = GetAnime(&AboutMenu);
        AddMenuEntry(&AboutMenu, &MenuEntryReturn);
    }
    
    RunMenu(&AboutMenu, NULL);
}

static VOID HelpRefit(VOID)
{
  if (HelpMenu.EntryCount == 0) {
    HelpMenu.TitleImage = BuiltinIcon(BUILTIN_ICON_FUNC_HELP);
    switch (gLanguage)
    {
      case russian:
        AddMenuInfoLine(&HelpMenu, L"ESC - Выход из подменю, обновление главного меню");
        AddMenuInfoLine(&HelpMenu, L"F1  - Помощь по горячим клавишам");
        AddMenuInfoLine(&HelpMenu, L"F2  - Сохранить отчет в preboot.log (только если FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F4  - Родной DSDT сохранить в EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Патченный DSDT сохранить в EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Сохранить ВидеоБиос в EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Снимок экрана в папку EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Извлечь указанный DVD");
        AddMenuInfoLine(&HelpMenu, L"Пробел - Подробнее о выбранном пункте");
        AddMenuInfoLine(&HelpMenu, L"Цифры 1-9 - Быстрый запуск тома по порядку в меню");
        AddMenuInfoLine(&HelpMenu, L"A - О загрузчике");
        AddMenuInfoLine(&HelpMenu, L"O - Дополнительные настройки");
        AddMenuInfoLine(&HelpMenu, L"R - Теплый перезапуск");
        AddMenuInfoLine(&HelpMenu, L"U - Выключить");
        break;
      case ukrainian:
        AddMenuInfoLine(&HelpMenu, L"ESC - Вийти з меню, оновити головне меню");
        AddMenuInfoLine(&HelpMenu, L"F1  - Ця довідка");
        AddMenuInfoLine(&HelpMenu, L"F2  - Зберегти preboot.log (т≥льки FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F4  - Зберегти OEM DSDT в EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Зберегти патчений DSDT в EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Зберегти VideoBios в EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Зберегти знімок екрану в EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Відкрити обраний диск (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Пробіл - докладніше про обраний пункт меню");
        AddMenuInfoLine(&HelpMenu, L"Клавіші 1-9 -  клавіші пунктів меню");
        AddMenuInfoLine(&HelpMenu, L"A - Про систему");
        AddMenuInfoLine(&HelpMenu, L"O - Опції меню");
        AddMenuInfoLine(&HelpMenu, L"R - Перезавантаження");
        AddMenuInfoLine(&HelpMenu, L"U - Відключити ПК");
        break;
      case spanish:
        AddMenuInfoLine(&HelpMenu, L"ESC - Salir de submenu o actualizar el menu principal");
        AddMenuInfoLine(&HelpMenu, L"F1  - Esta Ayuda");
        AddMenuInfoLine(&HelpMenu, L"F2  - Guardar preboot.log (Solo FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F4  - Guardar DSDT oem en EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Guardar DSDT parcheado en EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Guardar VideoBios en EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Guardar Captura de pantalla en EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Expulsar volumen seleccionado (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Espacio - Detalles acerca selected menu entry");
        AddMenuInfoLine(&HelpMenu, L"Digitos 1-9 - Atajo a la entrada del menu");
        AddMenuInfoLine(&HelpMenu, L"A - Menu Acerca de");
        AddMenuInfoLine(&HelpMenu, L"O - Menu Optiones");
        AddMenuInfoLine(&HelpMenu, L"R - Reiniciar Equipo");
        AddMenuInfoLine(&HelpMenu, L"U - Apagar");
        break;
      case portuguese:
        AddMenuInfoLine(&HelpMenu, L"ESC - Sai do submenu, atualiza o menu principal");
        AddMenuInfoLine(&HelpMenu, L"F1  - Esta ajuda");
        AddMenuInfoLine(&HelpMenu, L"F2  - Salva preboot.log (somente FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F4  - Salva oem DSDT em EFI/ACPI/origin/ (somente FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Salva DSDT corrigido em EFI/ACPI/origin/ (somente FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Salva VideoBios em EFI/misc/ (somente FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Salva screenshot em EFI/misc/ (somente FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Ejeta o volume selecionado (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Espaco - Detalhes sobre a opcao do menu selecionada");
        AddMenuInfoLine(&HelpMenu, L"Tecle 1-9 - Atalho para as entradas do menu");
        AddMenuInfoLine(&HelpMenu, L"A - Sobre o Menu");
        AddMenuInfoLine(&HelpMenu, L"O - Opcoes do Menu");
        AddMenuInfoLine(&HelpMenu, L"R - Reiniciar");
        AddMenuInfoLine(&HelpMenu, L"U - Desligar");
        break;
      case italian:
        AddMenuInfoLine(&HelpMenu, L"ESC - Esci dal submenu, Aggiorna menu principale");
        AddMenuInfoLine(&HelpMenu, L"F1  - Aiuto");
        AddMenuInfoLine(&HelpMenu, L"F2  - Salva il preboot.log (solo su FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F4  - Salva il DSDT oem in EFI/ACPI/origin/ (solo suFAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Salva il patched DSDT in EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Salva il VideoBios in EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Salva screenshot in EFI/misc/ (solo su FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Espelli il volume selezionato (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Spazio - Dettagli sul menu selezionato");
        AddMenuInfoLine(&HelpMenu, L"Digita 1-9 - Abbreviazioni per il menu");
        AddMenuInfoLine(&HelpMenu, L"A - Informazioni");
        AddMenuInfoLine(&HelpMenu, L"O - Menu Opzioni");
        AddMenuInfoLine(&HelpMenu, L"R - Riavvio");
        AddMenuInfoLine(&HelpMenu, L"U - Spegnimento");
        break;
      case german:
        AddMenuInfoLine(&HelpMenu, L"ESC - Zurueck aus Untermenue, Hauptmenue erneuern");
        AddMenuInfoLine(&HelpMenu, L"F1  - Diese Hilfe");
        AddMenuInfoLine(&HelpMenu, L"F2  - Sichere preboot.log (nur mit FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F4  - Sichere OEM DSDT in EFI/ACPI/origin/ (nur mit FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Sichere gepatchtes DSDT in EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Sichere VideoBios in EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Sichere Bildschirmfoto in EFI/misc/ (nur mit FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Volume auswerfen (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Leertaste - Details über den gewählten Menue Eintrag");
        AddMenuInfoLine(&HelpMenu, L"Zahlen 1-9 - Kurzwahl zum Menue Eintrag");
        AddMenuInfoLine(&HelpMenu, L"A - Menue Informationen");
        AddMenuInfoLine(&HelpMenu, L"O - Menue Optionen");
        AddMenuInfoLine(&HelpMenu, L"R - Neustart");
        AddMenuInfoLine(&HelpMenu, L"U - Ausschalten");
        break;		
      case french:
        AddMenuInfoLine(&HelpMenu, L"ESC - Quitter sous-menu, Retour menu principal");
        AddMenuInfoLine(&HelpMenu, L"F1  - Aide");
        AddMenuInfoLine(&HelpMenu, L"F2  - Enregistrer preboot.log (FAT32 only)");
        AddMenuInfoLine(&HelpMenu, L"F4  - Enregistrer oem DSDT dans EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Enregistrer DSDT modifié dans EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Enregistrer VideoBios dans EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Enregistrer la capture ecran dans EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Ejecter le volume (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Space - Details a propos du menu selectionné");
        AddMenuInfoLine(&HelpMenu, L"Digits 1-9 - Raccourci vers entrée menu");
        AddMenuInfoLine(&HelpMenu, L"A - A propos");
        AddMenuInfoLine(&HelpMenu, L"O - Options Menu");
        AddMenuInfoLine(&HelpMenu, L"R - Redemarrer");
        AddMenuInfoLine(&HelpMenu, L"U - Eteindre");
        break;
      case indonesian:
        AddMenuInfoLine(&HelpMenu, L"ESC - Keluar dari sub menu, merefresh main menu");
        AddMenuInfoLine(&HelpMenu, L"F1  - Tampilan Help ini");
        AddMenuInfoLine(&HelpMenu, L"F2  - Menimpan preboot.log (hanya untuk FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F4  - Menyimpan oem DSDT di EFI/ACPI/origin/ (hanya untuk FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Menyimpan patched DSDT di EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Menyimpan VideoBios di EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Menyimpan screenshot di EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Eject Volume yg terpilih (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Spasi - Detail dari menu yang terpilih");
        AddMenuInfoLine(&HelpMenu, L"Tombol 1-9 - Shorcut ke menu yang ada");
        AddMenuInfoLine(&HelpMenu, L"A - About menu");
        AddMenuInfoLine(&HelpMenu, L"O - Opsi menu");
        AddMenuInfoLine(&HelpMenu, L"R - Soft Reset");
        AddMenuInfoLine(&HelpMenu, L"U - Shutdown");
        break;
      case polish:
/*        AddMenuInfoLine(&HelpMenu, L"ESC - Wyjście z podmenu, Odświeżenie głównego menu");
        AddMenuInfoLine(&HelpMenu, L"F1  - Pomoc");
        AddMenuInfoLine(&HelpMenu, L"F2  - Zapis preboot.log (tylko dla FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F4  - Zapis czystych tabel DSDT do EFI/ACPI/origin/ (tylko dla FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Zapis zmodyfikowanego DSDT do EFI/ACPI/origin/ (tylko dla FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Zapis BIOSu karty graficznej do EFI/misc/ (tylko dla FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Zapis zrzutu ekranu do EFI/misc/ (tylko dla FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Wysunięcie zaznaczonego dysku (tylko dla DVD)");
        AddMenuInfoLine(&HelpMenu, L"Spacja - Informacje na temat dostępnych opcji dla zaznaczonego dysku");
        AddMenuInfoLine(&HelpMenu, L"Znaki 1-9 - Skróty do opcji dla zaznaczonego dysku");
        AddMenuInfoLine(&HelpMenu, L"A - Menu Informacyjne");
        AddMenuInfoLine(&HelpMenu, L"O - Menu Opcje");
        AddMenuInfoLine(&HelpMenu, L"R - Restart komputera");
        AddMenuInfoLine(&HelpMenu, L"U - Wyłączenie komputera");*/
        
        AddMenuInfoLine(&HelpMenu, L"ESC - Wyjscie z podmenu, Odswiezenie glownego menu");
        AddMenuInfoLine(&HelpMenu, L"F1  - Pomoc");
        AddMenuInfoLine(&HelpMenu, L"F2  - Zapis preboot.log (tylko FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F4  - Zapis DSDT do EFI/ACPI/origin/ (tylko FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Zapis poprawionego DSDT do EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Zapis BIOSu k. graficznej do EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Zapis zrzutu ekranu do EFI/misc/ (tylko FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Wysuniecie zaznaczonego dysku (tylko dla DVD)");
        AddMenuInfoLine(&HelpMenu, L"Spacja - Informacje nt. dostepnych opcji dla zaznaczonego dysku");
        AddMenuInfoLine(&HelpMenu, L"Znaki 1-9 - Skroty opcji dla wybranego dysku");
        AddMenuInfoLine(&HelpMenu, L"A - Menu Informacyjne");
        AddMenuInfoLine(&HelpMenu, L"O - Menu Opcje");
        AddMenuInfoLine(&HelpMenu, L"R - Restart komputera");
        AddMenuInfoLine(&HelpMenu, L"U - Wylaczenie komputera");
        break;
      case english:
      default:
        AddMenuInfoLine(&HelpMenu, L"ESC - Escape from submenu, Refresh main menu");
        AddMenuInfoLine(&HelpMenu, L"F1  - This help");
        AddMenuInfoLine(&HelpMenu, L"F2  - Save preboot.log (FAT32 only)");
        AddMenuInfoLine(&HelpMenu, L"F4  - Save oem DSDT into EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Save patched DSDT into EFI/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Save VideoBios into EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Save screenshot into EFI/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Eject selected volume (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Space - Details about selected menu entry");
        AddMenuInfoLine(&HelpMenu, L"Digits 1-9 - Shortcut to menu entry");
        AddMenuInfoLine(&HelpMenu, L"A - Menu About");
        AddMenuInfoLine(&HelpMenu, L"O - Menu Options");
        AddMenuInfoLine(&HelpMenu, L"R - Soft Reset");
        AddMenuInfoLine(&HelpMenu, L"U - Shutdown");
        break;
    }
    HelpMenu.AnimeRun = GetAnime(&HelpMenu);
    AddMenuEntry(&HelpMenu, &MenuEntryReturn);
  }
  
  RunMenu(&HelpMenu, NULL);
}


static EFI_STATUS StartEFIImageList(IN EFI_DEVICE_PATH **DevicePaths,
                                    IN CHAR16 *LoadOptions, IN CHAR16 *LoadOptionsPrefix,
                                    IN CHAR16 *ImageTitle,
                                    OUT UINTN *ErrorInStep,
                                    OUT EFI_HANDLE *NewImageHandle)
{
  EFI_STATUS              Status, ReturnStatus;
  EFI_HANDLE              ChildImageHandle = 0;
  EFI_LOADED_IMAGE        *ChildLoadedImage;
  UINTN                   DevicePathIndex;
  CHAR16                  ErrorInfo[256];
  CHAR16                  *FullLoadOptions = NULL;
  
  DBG("Starting %s\n", ImageTitle);
  if (ErrorInStep != NULL) {
    *ErrorInStep = 0;
  }
  if (NewImageHandle != NULL) {
    *NewImageHandle = NULL;
  }
  
  // load the image into memory
  ReturnStatus = Status = EFI_NOT_FOUND;  // in case the list is empty
  for (DevicePathIndex = 0; DevicePaths[DevicePathIndex] != NULL; DevicePathIndex++) {
    ReturnStatus = Status = gBS->LoadImage(FALSE, SelfImageHandle, DevicePaths[DevicePathIndex], NULL, 0, &ChildImageHandle);
    if (ReturnStatus != EFI_NOT_FOUND)
      break;
  }
  UnicodeSPrint(ErrorInfo, 512, L"while loading %s", ImageTitle);
  if (CheckError(Status, ErrorInfo)) {
    if (ErrorInStep != NULL)
      *ErrorInStep = 1;
    goto bailout;
  }
  
  // set load options
  if (LoadOptions != NULL) {
    ReturnStatus = Status = gBS->HandleProtocol(ChildImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &ChildLoadedImage);
    if (CheckError(Status, L"while getting a LoadedImageProtocol handle")) {
      if (ErrorInStep != NULL)
        *ErrorInStep = 2;
      goto bailout_unload;
    }
    
    if (LoadOptionsPrefix != NULL) {
      FullLoadOptions = PoolPrint(L"%s %s ", LoadOptionsPrefix, LoadOptions);
      // NOTE: That last space is also added by the EFI shell and seems to be significant
      //  when passing options to Apple's boot.efi...
      LoadOptions = FullLoadOptions;
    }
    // NOTE: We also include the terminating null in the length for safety.
    ChildLoadedImage->LoadOptions = (VOID *)LoadOptions;
    ChildLoadedImage->LoadOptionsSize = (UINT32)StrSize(LoadOptions);
    //((UINT32)StrLen(LoadOptions) + 1) * sizeof(CHAR16);
    DBG("Using load options '%s'\n", LoadOptions);
  }
  //DBG("Image loaded at: %p\n", ChildLoadedImage->ImageBase);
  //PauseForKey(L"continue");
  
  // close open file handles
  UninitRefitLib();
  
  // turn control over to the image
  //
  // Before calling the image, enable the Watchdog Timer for
  // the 5 Minute period - Slice - NO! 60seconds is enough
  //  
  gBS->SetWatchdogTimer (180, 0x0000, 0x00, NULL);
  
  ReturnStatus = Status = gBS->StartImage(ChildImageHandle, NULL, NULL);
  //
  // Clear the Watchdog Timer after the image returns
  //
  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);
  
  //PauseForKey(L"Returned from StartImage\n");
  
  // control returns here when the child image calls Exit()
  if (ImageTitle) {
    UnicodeSPrint(ErrorInfo, 512, L"returned from %s", ImageTitle);
  }
  
  if (CheckError(Status, ErrorInfo)) {
    if (ErrorInStep != NULL)
      *ErrorInStep = 3;
  }
//  PauseForKey(L"Error checked\n");
  // re-open file handles
//  Status = ReinitRefitLib();  
//  PauseForKey(L"ReinitRefitLib OK\n");
  //Slice
/*  if (EFI_ERROR(Status)) {
    goto bailout_unload;
  }
 */
  if (!EFI_ERROR(ReturnStatus)) { //why unload driver?!
    if (NewImageHandle != NULL) {
      *NewImageHandle = ChildImageHandle;
    }
    goto bailout;
  }
  
bailout_unload:
  // unload the image, we don't care if it works or not...
  Status = gBS->UnloadImage(ChildImageHandle);
  if (FullLoadOptions != NULL)
    FreePool(FullLoadOptions);
bailout:
  return ReturnStatus;
}

static EFI_STATUS StartEFIImage(IN EFI_DEVICE_PATH *DevicePath,
                                IN CHAR16 *LoadOptions, IN CHAR16 *LoadOptionsPrefix,
                                IN CHAR16 *ImageTitle,
                                OUT UINTN *ErrorInStep,
                                OUT EFI_HANDLE *NewImageHandle)
{
  EFI_DEVICE_PATH *DevicePaths[2];
  
  DevicePaths[0] = DevicePath;
  DevicePaths[1] = NULL;
  return StartEFIImageList(DevicePaths, LoadOptions, LoadOptionsPrefix, ImageTitle, ErrorInStep, NewImageHandle);
}

//
// Null ConOut OutputString() implementation - for blocking
// text output from boot.efi when booting in graphics mode
//
EFI_STATUS EFIAPI
NullConOutOutputString(IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, IN CHAR16 *String) {
	return EFI_SUCCESS;
}

//
// EFI OS loader functions
//
EG_PIXEL DarkBackgroundPixel  = { 0x0, 0x0, 0x0, 0x0 };

static VOID StartLoader(IN LOADER_ENTRY *Entry)
{
  EFI_STATUS              Status;
  BOOLEAN                 BlockConOut = FALSE;
  EFI_TEXT_STRING         ConOutOutputString = 0;
  
  DBGT("StartLoader() start\n");
  egClearScreen(&DarkBackgroundPixel);
  MsgLog("Turbo=%c\n", gSettings.Turbo?'Y':'N');
//  MsgLog("PatchAPIC=%c\n", gSettings.PatchNMI?'Y':'N');
//  MsgLog("PatchVBios=%c\n", gSettings.PatchVBios?'Y':'N');
//  DBG("KillMouse\n");
  KillMouse();
//    DBG("BeginExternalScreen\n");
  BeginExternalScreen(Entry->UseGraphicsMode, L"Booting OS");
  if (Entry->LoaderType == OSTYPE_OSX) {
    // first patchACPI and find PCIROOT and RTC
    // but before ACPI patch we need smbios patch
//    DBG("PatchSmbios\n");
    
    ApplySettings();
    PatchSmbios();
//    DBG("PatchACPI\n");
    PatchACPI(Entry->Volume);
//DBG("GetOSVersion\n");
    Status = GetOSVersion(Entry->Volume);
    
    Entry->LoadOptions     = PoolPrint(L"%a", gSettings.BootArgs); //moved here, before using Entry ;)
//    DBG("SetDevices\n");
    SetDevices();
//    DBG("SetFSInjection\n");
    SetFSInjection(Entry);
    //PauseForKey(L"SetFSInjection");
//    DBG("SetVariablesForOSX\n");
    SetVariablesForOSX();
//    DBG("SetVariablesForOSX\n");
    EventsInitialize();
//    DBG("FinalizeSmbios\n");
    FinalizeSmbios();
//    DBG("SetupDataForOSX\n");
    SetupDataForOSX();
//    DBG("LoadKexts\n");
    LoadKexts(Entry);
//    DBG("SetupBooterLog\n");
    DBGT("Closing log\n");
    Status = SetupBooterLog();
    
    // blocking boot.efi output if -v is not specified
    // note: this blocks output even if -v is specified in
    // /Library/Preferences/SystemConfiguration/com.apple.Boot.plist
    // which is wrong
    if (Entry->LoadOptions == NULL || StrStr(Entry->LoadOptions, L"-v") == NULL) {
      BlockConOut = TRUE;
    }
  }
  else if ((Entry->LoaderType == OSTYPE_WIN) ||  (Entry->LoaderType == OSTYPE_WINEFI)){
      
    PatchACPI_OtherOS(L"Windows", FALSE);
    //PauseForKey(L"continue");
      
  }
  else if (Entry->LoaderType == OSTYPE_LIN) {
      FinalizeSmbios();
      PatchACPI_OtherOS(L"Linux", FALSE);
      //PauseForKey(L"continue");
      Entry->LoadOptions     = NULL;
  }
  
  SetStartupDiskVolume(Entry->Volume, Entry->LoaderType == OSTYPE_OSX ? NULL : Entry->LoaderPath);
  
  if (BlockConOut) {
    // save orig OutputString and replace it with
    // null implementation
    ConOutOutputString = gST->ConOut->OutputString;
    gST->ConOut->OutputString = NullConOutOutputString;
  }
//  DBG("StartEFIImage\n");
  StartEFIImage(Entry->DevicePath, Entry->LoadOptions,
                Basename(Entry->LoaderPath), Basename(Entry->LoaderPath), NULL, NULL);
  
  if (BlockConOut) {
    // return back orig OutputString
    gST->ConOut->OutputString = ConOutOutputString;
  }
  
//  PauseForKey(L"FinishExternalScreen");
  FinishExternalScreen();
//  PauseForKey(L"System started?!");
}

static LOADER_ENTRY * AddLoaderEntry(IN CHAR16 *LoaderPath, IN CHAR16 *LoaderTitle, IN REFIT_VOLUME *Volume, UINT8 OSType)
{
  CHAR16            *FileName, *OSIconName;
  CHAR16            IconFileName[256];
  CHAR16            DiagsFileName[256];
  CHAR16            ShortcutLetter;
  UINTN             LoaderKind;
  LOADER_ENTRY      *Entry, *SubEntry;
  REFIT_MENU_SCREEN *SubScreen;
  UINT64            VolumeSize;
  BOOLEAN           UsesSlideArg;
  
  FileName = Basename(LoaderPath);
  
  // prepare the menu entry
  Entry = AllocateZeroPool(sizeof(LOADER_ENTRY));
  Entry->me.Title        = PoolPrint(L"Boot %s from %s", (LoaderTitle != NULL) ? LoaderTitle : LoaderPath + 1, Volume->VolName);
  if (Volume->BootType == BOOTING_BY_EFI) {
    Entry->me.Tag          = TAG_LOADER;
  } else {
    Entry->me.Tag          = TAG_LEGACY;
  }

  Entry->me.Row          = 0;
  Entry->Volume = Volume;
//  DBG("HideBadges=%d Volume=%s\n", GlobalConfig.HideBadges, Volume->VolName);
  if ((GlobalConfig.HideBadges == HDBADGES_NONE) || 
      (GlobalConfig.HideBadges == HDBADGES_INT && Volume->DiskKind != DISK_KIND_INTERNAL)){
    Entry->me.BadgeImage   = egCopyScaledImage(Volume->OSImage, 8);
  } else if (GlobalConfig.HideBadges == HDBADGES_SWAP) { 
    Entry->me.BadgeImage   =  egCopyScaledImage(Volume->DriveImage, 4);
  }
  Entry->LoaderPath      = EfiStrDuplicate(LoaderPath);
  Entry->VolName         = Volume->VolName;
  Entry->DevicePath      = FileDevicePath(Volume->DeviceHandle, Entry->LoaderPath);
  Entry->UseGraphicsMode = FALSE;
  Entry->LoadOptions     = PoolPrint(L"%a", gSettings.BootArgs);
//  Entry->LoadOptions     = InputItems[0].SValue;

  // locate a custom icon for the loader
//  StrCpy(IconFileName, LoaderPath);
  StrCpy(IconFileName, Volume->OSIconName);
//  ReplaceExtension(IconFileName, L".icns");
/*  if (FileExists(Volume->RootDir, IconFileName)){
    Entry->me.Image = LoadIcns(Volume->RootDir, IconFileName, 128);
  } else if (FileExists(SelfRootDir, IconFileName)) {
    Entry->me.Image = LoadIcns(SelfRootDir, IconFileName, 128);
  } */
  //actions
  Entry->me.AtClick = ActionSelect;
  Entry->me.AtDoubleClick = ActionEnter;
  Entry->me.AtRightClick = ActionDetails;
  
  // detect specific loaders
  OSIconName = NULL;
  LoaderKind = 0;
  ShortcutLetter = 0;
  /*
  if (StriCmp(LoaderPath, MACOSX_LOADER_PATH) == 0) {
    OSIconName = Volume->OSIconName;
    Entry->UseGraphicsMode = TRUE;
    LoaderKind = 1;
    ShortcutLetter = 'M';
  } else if (StriCmp(FileName, L"diags.efi") == 0) {
    OSIconName = L"hwtest";
  } else if (StriCmp(FileName, L"e.efi") == 0 ||
             StriCmp(FileName, L"elilo.efi") == 0) {
    OSIconName = L"elilo,linux";
    LoaderKind = 2;
    ShortcutLetter = 'L';
  } else if (StriCmp(FileName, L"cdboot.efi") == 0 ||
             StriCmp(FileName, L"bootmgr.efi") == 0 ||
             StriCmp(FileName, L"Bootmgfw.efi") == 0) {
    OSIconName = L"win";
    ShortcutLetter = 'W';
  } else if (StriCmp(FileName, L"xom.efi") == 0) {
    OSIconName = L"xom,win";
    Entry->UseGraphicsMode = TRUE;
    LoaderKind = 3;
    ShortcutLetter = 'W';
  }
   */
  switch (OSType) {
    case OSTYPE_OSX:
    case OSTYPE_TIGER:
    case OSTYPE_LEO:
    case OSTYPE_SNOW:
    case OSTYPE_LION:
    case OSTYPE_COUGAR:
    case OSTYPE_RECOVERY:
      OSIconName = Volume->OSIconName;
      Entry->UseGraphicsMode = TRUE;
      LoaderKind = 1;
      ShortcutLetter = 'M';    
      Entry->LoaderType = OSTYPE_OSX;
      break;
    case OSTYPE_WIN:
      OSIconName = L"win";
      ShortcutLetter = 'W';
      LoaderKind = 3;
      Entry->LoaderType = OSTYPE_WIN;
      break;
    case OSTYPE_WINEFI:
      OSIconName = L"vista";
      ShortcutLetter = 'V';
      LoaderKind = 3;
      Entry->LoaderType = OSTYPE_WINEFI;
      break;
    case OSTYPE_LIN:
      OSIconName = L"linux";
      LoaderKind = 2;
      ShortcutLetter = 'L';
      Entry->LoaderType = OSTYPE_LIN;
      break;
    case OSTYPE_VAR:
    case OSTYPE_EFI:
      OSIconName = L"clover";
      LoaderKind = 4;
      ShortcutLetter = 'U';
      Entry->LoaderType = OSTYPE_VAR;
      break;
    default:
      OSIconName = L"unknown";
      Entry->LoaderType = OSTYPE_VAR;
      break;
  }
  Entry->me.ShortcutLetter = ShortcutLetter;
//  if (Entry->me.Image == NULL)
  Entry->me.Image = LoadOSIcon(OSIconName, L"unknown", FALSE);
  
  // create the submenu
  SubScreen = AllocateZeroPool(sizeof(REFIT_MENU_SCREEN));
  SubScreen->Title = PoolPrint(L"Boot Options for %s on %s", (LoaderTitle != NULL) ? LoaderTitle : FileName, Volume->VolName);
  SubScreen->TitleImage = Entry->me.Image;
  SubScreen->ID = OSType + 20;
//  DBG("get anime for os=%d\n", SubScreen->ID);
  SubScreen->AnimeRun = GetAnime(SubScreen);
  VolumeSize = MultU64x32 (Volume->BlockIO->Media->LastBlock, Volume->BlockIO->Media->BlockSize) >> 20;
  AddMenuInfoLine(SubScreen, PoolPrint(L"Volume size: %dMb", VolumeSize));
  
  // Aptio UEFI ML boot requires slide=0
  // if user have it in BootArgs, then propagate it to submenu entries
  UsesSlideArg = AsciiStrStr(gSettings.BootArgs, "slide=0") != 0;
  
  // default entry
  SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
  SubEntry->me.Title        = (LoaderKind == 1) ? L"Boot Mac OS X" : PoolPrint(L"Run %s", FileName);
  SubEntry->me.Tag          = TAG_LOADER;
  SubEntry->LoaderPath      = Entry->LoaderPath;
  SubEntry->Volume          = Entry->Volume;
  SubEntry->VolName         = Entry->VolName;
  SubEntry->DevicePath      = Entry->DevicePath;
  SubEntry->UseGraphicsMode = Entry->UseGraphicsMode;
  SubEntry->LoadOptions     = PoolPrint(L"%a", gSettings.BootArgs);
  SubEntry->LoaderType      = Entry->LoaderType;
  SubEntry->me.AtClick      = ActionEnter;
  AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
  
  // loader-specific submenu entries
  if (LoaderKind == 1) {          // entries for Mac OS X
#if defined(MDE_CPU_X64)
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Boot Mac OS X with a 64-bit kernel";
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->UseGraphicsMode = Entry->UseGraphicsMode;
    SubEntry->LoadOptions     = UsesSlideArg ? L"arch=x86_64 slide=0" : L"arch=x86_64";
    SubEntry->LoaderType      = OSTYPE_OSX;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Boot Mac OS X with a 32-bit kernel";
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->UseGraphicsMode = Entry->UseGraphicsMode;
    SubEntry->LoadOptions     = L"arch=i386";
    SubEntry->LoaderType      = OSTYPE_OSX;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
#endif
    
    if (!(GlobalConfig.DisableFlags & DISABLE_FLAG_SINGLEUSER)) {
      SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
      SubEntry->me.Title        = L"Boot Mac OS X in verbose mode";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = Entry->LoaderPath;
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = Entry->DevicePath;
      SubEntry->UseGraphicsMode = FALSE;
      SubEntry->LoadOptions     = UsesSlideArg ? L"-v slide=0" : L"-v";
      SubEntry->LoaderType      = OSTYPE_OSX;
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
      
#if defined(MDE_CPU_X64)
      SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
      SubEntry->me.Title        = L"Boot Mac OS X in verbose mode (64-bit)";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = Entry->LoaderPath;
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = Entry->DevicePath;
      SubEntry->UseGraphicsMode = FALSE;
      SubEntry->LoadOptions     = UsesSlideArg ? L"-v arch=x86_64 slide=0" : L"-v arch=x86_64";
      SubEntry->LoaderType      = OSTYPE_OSX;
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
      
      SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
      SubEntry->me.Title        = L"Boot Mac OS X in verbose mode (32-bit)";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = Entry->LoaderPath;
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = Entry->DevicePath;
      SubEntry->UseGraphicsMode = FALSE;
      SubEntry->LoadOptions     = L"-v arch=i386";
      SubEntry->LoaderType      = OSTYPE_OSX;
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
#endif
      
      SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
      SubEntry->me.Title        = L"Boot Mac OS X in single user mode";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = Entry->LoaderPath;
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = Entry->DevicePath;
      SubEntry->UseGraphicsMode = FALSE;
      SubEntry->LoadOptions     = UsesSlideArg ? L"-v -s slide=0" : L"-v -s";
      SubEntry->LoaderType      = OSTYPE_OSX;
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
      
      SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
      SubEntry->me.Title        = L"Boot Mac OS X with extra kexts (skips cache)";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = Entry->LoaderPath;
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = Entry->DevicePath;
      SubEntry->UseGraphicsMode = FALSE;
      SubEntry->LoadOptions     = UsesSlideArg ? L"-v slide=0 WithKexts" : L"-v WithKexts"; //default arch 10.6->32bit, 10.7->64bit
      SubEntry->LoaderType      = OSTYPE_OSX;
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    }
    
    // check for Apple hardware diagnostics
    StrCpy(DiagsFileName, L"\\System\\Library\\CoreServices\\.diagnostics\\diags.efi");
    if (FileExists(Volume->RootDir, DiagsFileName) && !(GlobalConfig.DisableFlags & DISABLE_FLAG_HWTEST)) {
      DBG("  - Apple Hardware Test found\n");
      
      SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
      SubEntry->me.Title        = L"Run Apple Hardware Test";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = EfiStrDuplicate(DiagsFileName);
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = FileDevicePath(Volume->DeviceHandle, SubEntry->LoaderPath);
      SubEntry->UseGraphicsMode = TRUE;
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    }
    
  } else if (LoaderKind == 2) {   // entries for elilo
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = PoolPrint(L"Run %s in interactive mode", FileName);
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->UseGraphicsMode = Entry->UseGraphicsMode;
    SubEntry->LoadOptions     = L"-p";
    SubEntry->LoaderType      = OSTYPE_LIN;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Boot Linux for a 17\" iMac or a 15\" MacBook Pro (*)";
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->UseGraphicsMode = TRUE;
    SubEntry->LoadOptions     = L"-d 0 i17";
    SubEntry->LoaderType      = OSTYPE_LIN;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Boot Linux for a 20\" iMac (*)";
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->UseGraphicsMode = TRUE;
    SubEntry->LoadOptions     = L"-d 0 i20";
    SubEntry->LoaderType      = OSTYPE_LIN;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Boot Linux for a Mac Mini (*)";
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->UseGraphicsMode = TRUE;
    SubEntry->LoadOptions     = L"-d 0 mini";
    SubEntry->LoaderType      = OSTYPE_LIN;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    AddMenuInfoLine(SubScreen, L"NOTE: This is an example. Entries");
    AddMenuInfoLine(SubScreen, L"marked with (*) may not work.");
    
  } else if (LoaderKind == 3) {   // entries for xom.efi
                                  // by default, skip the built-in selection and boot from hard disk only
    Entry->LoadOptions = L"-s -h";
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Boot Windows from Hard Disk";
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->UseGraphicsMode = Entry->UseGraphicsMode;
    SubEntry->LoadOptions     = L"-s -h";
    SubEntry->LoaderType      = OSTYPE_WIN;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Boot Windows from CD-ROM";
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->UseGraphicsMode = Entry->UseGraphicsMode;
    SubEntry->LoadOptions     = L"-s -c";
    SubEntry->LoaderType      = OSTYPE_WIN;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = PoolPrint(L"Run %s in text mode", FileName);
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->UseGraphicsMode = FALSE;
    SubEntry->LoadOptions     = L"-v";
    SubEntry->LoaderType      = OSTYPE_VAR;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
  }
  
  AddMenuEntry(SubScreen, &MenuEntryReturn);
  Entry->me.SubScreen = SubScreen;
  AddMenuEntry(&MainMenu, (REFIT_MENU_ENTRY *)Entry);
  return Entry;
}

static LOADER_ENTRY * AddCloverEntry(IN CHAR16 *LoaderPath, IN CHAR16 *LoaderTitle, IN REFIT_VOLUME *Volume)
{
  // CHAR16            *FileName, *OSIconName;
  // CHAR16            IconFileName[256];
  // CHAR16            ShortcutLetter;
  // UINTN             LoaderKind;
  LOADER_ENTRY      *Entry, *SubEntry;
  REFIT_MENU_SCREEN *SubScreen;
  EFI_STATUS        Status;
  
  // FileName = Basename(LoaderPath);
  
  // prepare the menu entry
  Entry = AllocateZeroPool(sizeof(LOADER_ENTRY));
  Entry->me.Title          = LoaderTitle;
  Entry->me.Tag            = TAG_CLOVER;
  Entry->me.Row            = 1;
  Entry->me.ShortcutLetter = 'C';
  Entry->me.Image          = BuiltinIcon(BUILTIN_ICON_FUNC_CLOVER);
  Entry->Volume = Volume;
  //  DBG("HideBadges=%d Volume=%s\n", GlobalConfig.HideBadges, Volume->VolName);
/*  if ((GlobalConfig.HideBadges == HDBADGES_NONE) ||
      (GlobalConfig.HideBadges == HDBADGES_INT && Volume->DiskKind != DISK_KIND_INTERNAL)){
    Entry->me.BadgeImage   = egCopyScaledImage(Volume->OSImage, 8);
  } else if (GlobalConfig.HideBadges == HDBADGES_SWAP) {
    Entry->me.BadgeImage   =  egCopyScaledImage(Volume->DriveImage, 4);
  }*/
//  Entry->me.Image   = egCopyScaledImage(Volume->OSImage, 4);
  Entry->LoaderPath      = EfiStrDuplicate(LoaderPath);
  Entry->VolName         = Volume->VolName;
  Entry->DevicePath      = FileDevicePath(Volume->DeviceHandle, Entry->LoaderPath);
  Entry->UseGraphicsMode = FALSE;
  Entry->LoadOptions     = NULL;
  
  // locate a custom icon for the loader
  // StrCpy(IconFileName, Volume->OSIconName);
  
  //actions
  Entry->me.AtClick = ActionSelect;
  Entry->me.AtDoubleClick = ActionDetails;
  Entry->me.AtRightClick = ActionDetails;
  
  // OSIconName = L"clover";
  // LoaderKind = 5;
  // ShortcutLetter = 'C';
  // Entry->me.Tag     = TAG_CLOVER;
  // Entry->me.ShortcutLetter = ShortcutLetter;
  //  if (Entry->me.Image == NULL)
  // Entry->me.BadgeImage = LoadOSIcon(OSIconName, L"unknown", FALSE);
  // Entry->me.Image   = egCopyScaledImage(Entry->me.BadgeImage, 6);
  
  // create the submenu
  SubScreen = AllocateZeroPool(sizeof(REFIT_MENU_SCREEN));
  SubScreen->Title = EfiStrDuplicate(LoaderTitle);
  SubScreen->TitleImage = Entry->me.Image;
  SubScreen->ID = SCREEN_BOOT;
  SubScreen->AnimeRun = GetAnime(SubScreen);
  AddMenuInfoLine(SubScreen, DevicePathToStr(Volume->DevicePath));
  
  Status = FindBootOptionForFile (Entry->Volume->DeviceHandle,
                                  Entry->LoaderPath,
                                  NULL,
                                  NULL
                                  );
  if (Status == EFI_SUCCESS) {
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Remove as UEFI boot option";
    SubEntry->me.Tag          = TAG_CLOVER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->UseGraphicsMode = FALSE;
    SubEntry->LoadOptions     = L"BO-REMOVE";
    SubEntry->LoaderType      = OSTYPE_VAR;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
  } else {
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Add as UEFI boot option";
    SubEntry->me.Tag          = TAG_CLOVER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->UseGraphicsMode = FALSE;
    SubEntry->LoadOptions     = L"BO-ADD";
    SubEntry->LoaderType      = OSTYPE_VAR;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
  }
  
  SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
  SubEntry->me.Title        = L"Print all UEFI boot options to log";
  SubEntry->me.Tag          = TAG_CLOVER;
  SubEntry->LoaderPath      = Entry->LoaderPath;
  SubEntry->Volume          = Entry->Volume;
  SubEntry->VolName         = Entry->VolName;
  SubEntry->DevicePath      = Entry->DevicePath;
  SubEntry->UseGraphicsMode = FALSE;
  SubEntry->LoadOptions     = L"BO-PRINT";
  SubEntry->LoaderType      = OSTYPE_VAR;
  SubEntry->me.AtClick      = ActionEnter;
  AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
  AddMenuEntry(SubScreen, &MenuEntryReturn);
  Entry->me.SubScreen = SubScreen;
  AddMenuEntry(&MainMenu, (REFIT_MENU_ENTRY *)Entry);
  return Entry;
}

/*
static VOID ScanLoaderDir(IN REFIT_VOLUME *Volume, IN CHAR16 *Path)
{
  EFI_STATUS              Status;
  REFIT_DIR_ITER          DirIter;
  EFI_FILE_INFO           *DirEntry;
  CHAR16                  FileName[256];
  
  // look through contents of the directory
  DirIterOpen(Volume->RootDir, Path, &DirIter);
  while (DirIterNext(&DirIter, 2, L"*.efi", &DirEntry)) {
    if (DirEntry->FileName[0] == '.' ||
        StriCmp(DirEntry->FileName, L"TextMode.efi") == 0 ||
        StriCmp(DirEntry->FileName, L"ebounce.efi") == 0 ||
        StriCmp(DirEntry->FileName, L"GraphicsConsole.efi") == 0)
      continue;   // skip this
    
    if (Path)
      UnicodeSPrint(FileName, 512, L"\\%s\\%s", Path, DirEntry->FileName);
    else
      UnicodeSPrint(FileName, 512, L"\\%s", DirEntry->FileName);
//    AddLoaderEntry(FileName, NULL, Volume);
  }
  Status = DirIterClose(&DirIter);
  if (Status != EFI_NOT_FOUND) {
    if (Path)
      UnicodeSPrint(FileName, 512, L"while scanning the %s directory", Path);
    else
      StrCpy(FileName, L"while scanning the root directory");
    CheckError(Status, FileName);
  }
}
*/
static VOID ScanLoader(VOID)
{
  UINTN                   VolumeIndex;
  REFIT_VOLUME            *Volume;
  CHAR16                  FileName[256];
  LOADER_ENTRY            *Entry;
  CHAR16                  VolumeString[256];
  INT32                   HVi;
  CHAR16                  *HV;
//  EFI_STATUS              Status;
  
  //    Print(L"Scanning for boot loaders...\n");
  
  for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
    Volume = Volumes[VolumeIndex];
    if (Volume->RootDir == NULL) { // || Volume->VolName == NULL)
      DBG("Volume %d has no root\n", VolumeIndex);
      continue;
    }
    if (Volume->VolName == NULL) {
      DBG("Volume %d has no name\n", VolumeIndex);
      Volume->VolName = L"EFI volume";
    }
    
    // skip volume if its kind is configured as disabled
    if ((Volume->DiskKind == DISK_KIND_OPTICAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_OPTICAL)) ||
        (Volume->DiskKind == DISK_KIND_EXTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_EXTERNAL)) ||
        (Volume->DiskKind == DISK_KIND_INTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_INTERNAL)))
      continue;
    
    StrCpy(VolumeString, DevicePathToStr(Volume->DevicePath));
    HV = NULL;
	for (HVi = 0; HVi < gSettings.HVCount; HVi++) {
	  HV = StrStr(VolumeString, gSettings.HVHideStrings[HVi]);
	  if (HV != NULL) break;
	}
	if (HV != NULL)
	  continue;
    
    // check for Mac OS X boot loader
    StrCpy(FileName, MACOSX_LOADER_PATH);
    if (FileExists(Volume->RootDir, FileName)) {
      //     Print(L"  - Mac OS X boot file found\n");
      Volume->BootType = BOOTING_BY_EFI;
      Volume->DriveImage = ScanVolumeDefaultIcon(Volume);
      if (!gSettings.HVHideAllOSX)
        Entry = AddLoaderEntry(FileName, L"Mac OS X", Volume, Volume->OSType);
      //     continue; //boot MacOSX only
    }
//crazybirdy
    //============ add in begin ============
    // check for Mac OS X Install Data
    StrCpy(FileName, L"\\OS X Install Data\\boot.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->BootType = BOOTING_BY_EFI;
      Volume->OSType = OSTYPE_COUGAR;
      Volume->OSIconName = L"cougar";
      if (!gSettings.HVHideAllOSXInstall)
      Entry = AddLoaderEntry(FileName, L"OS X Install", Volume, Volume->OSType);
      continue; //boot MacOSX only
    }
    // check for Mac OS X Install Data
    StrCpy(FileName, L"\\Mac OS X Install Data\\boot.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->BootType = BOOTING_BY_EFI;
      Volume->OSType = OSTYPE_LION;
      Volume->OSIconName = L"lion";      
      if (!gSettings.HVHideAllOSXInstall) {
        Entry = AddLoaderEntry(FileName, L"Mac OS X Install", Volume, Volume->OSType);
        continue; //boot MacOSX only
      }
    }
    // dmazar: ML install from Lion to empty partition
    // starting (Lion) partition: /.IABootFiles with boot.efi and kernelcache,
    // and with DMGs used from Install app.
    // destination partition: just logs and config
    StrCpy(FileName, L"\\.IABootFiles\\boot.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->BootType = BOOTING_BY_EFI;
      Volume->OSType = OSTYPE_COUGAR;
      Volume->OSIconName = L"cougar";
      if (!gSettings.HVHideAllOSXInstall) {
        Entry = AddLoaderEntry(FileName, L"OS X Install", Volume, Volume->OSType);
        //continue; //boot MacOSX only
      }
    }
    //============ add in end ============
    
    // check for Mac OS X Recovery Boot
    StrCpy(FileName,  L"\\com.apple.recovery.boot\\boot.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->BootType = BOOTING_BY_EFI;
      Volume->OSType = OSTYPE_RECOVERY; 
      Volume->OSIconName = L"mac";
      Volume->DriveImage = ScanVolumeDefaultIcon(Volume);
      if (!gSettings.HVHideAllRecovery) {
        Entry = AddLoaderEntry(FileName, L"Recovery", Volume, Volume->OSType);
        continue; //boot recovery only
      }
    }
    
    // check for XOM - and what?
    //    StrCpy(FileName, L"\\System\\Library\\CoreServices\\xom.efi");
    /*        StrCpy(FileName, L"\\EFI\\tools\\xom.efi");
     if (FileExists(Volume->RootDir, FileName)) {
     Volume->BootType = BOOTING_BY_EFI;
     AddLoaderEntry(L"Xom.efi", L"Windows XP ", Volume, OSTYPE_WIN);
     }*/
    
    // check for Microsoft boot loader/menu
    StrCpy(FileName, L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      //     Print(L"  - Microsoft boot menu found\n");
      Volume->OSType = OSTYPE_WINEFI;
      Volume->BootType = BOOTING_BY_EFI;
      Volume->DriveImage = ScanVolumeDefaultIcon(Volume);
      if (!gSettings.HVHideAllWindowsEFI){
        Entry = AddLoaderEntry(FileName, L"Microsoft EFI boot menu", Volume, OSTYPE_WINEFI);
 //     continue;
      }
    }
    
    // check for Microsoft boot loader/menu
    StrCpy(FileName, L"\\bootmgr.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      //     Print(L"  - Microsoft boot menu found\n");
      Volume->OSType = OSTYPE_WINEFI;
      Volume->BootType = BOOTING_BY_EFI;
      Volume->DriveImage = ScanVolumeDefaultIcon(Volume);
      if (!gSettings.HVHideAllWindowsEFI){
        Entry = AddLoaderEntry(FileName, L"Microsoft EFI boot menu", Volume, OSTYPE_WINEFI);
        continue;
      }
    }
 
    // check for Microsoft boot loader/menu on CDROM
    StrCpy(FileName, L"\\EFI\\MICROSOFT\\BOOT\\cdboot.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      //     Print(L"  - Microsoft boot menu found\n");
      Volume->OSType = OSTYPE_WINEFI;
      Volume->BootType = BOOTING_BY_EFI;
      Volume->DriveImage = ScanVolumeDefaultIcon(Volume);
      if (!gSettings.HVHideAllWindowsEFI){
        Entry = AddLoaderEntry(FileName, L"Microsoft EFI boot menu", Volume, OSTYPE_WINEFI);
        continue;
      }
    }
    

    // check for grub boot loader/menu
#if defined(MDE_CPU_X64)
      StrCpy(FileName, L"\\EFI\\grub\\grubx64.efi");
#else
      StrCpy(FileName, L"\\EFI\\grub\\grub.efi");
#endif
      
      if (FileExists(Volume->RootDir, FileName)) {
        Volume->OSType = OSTYPE_LIN;
        Volume->BootType = BOOTING_BY_EFI;
        Volume->DriveImage = ScanVolumeDefaultIcon(Volume);  
        if (!gSettings.HVHideAllGrub) {
          Entry = AddLoaderEntry(FileName, L"Grub EFI boot menu", Volume, OSTYPE_LIN);
 //     continue;
        }
    }
      // check for Gentoo boot loader/menu
#if defined(MDE_CPU_X64)
      StrCpy(FileName, L"\\EFI\\Gentoo\\grubx64.efi");
#else
      StrCpy(FileName, L"\\EFI\\Gentoo\\grub.efi");
#endif
      if (FileExists(Volume->RootDir, FileName)) {
          Volume->BootType = BOOTING_BY_EFI;
          if (!gSettings.HVHideAllGentoo)
          Entry = AddLoaderEntry(FileName, L"Gentoo EFI boot menu", Volume, OSTYPE_LIN);
      }
    
    // check for Gentoo kernel
#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\Gentoo\\kernelx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\Gentoo\\kernel.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->BootType = BOOTING_BY_EFI;
      if (!gSettings.HVHideAllGentoo)
      Entry = AddLoaderEntry(FileName, L"Gentoo EFI kernel", Volume, OSTYPE_LIN);
    }
    
    // check for Redhat boot loader/menu
    StrCpy(FileName, L"\\EFI\\RedHat\\grub.efi");
    if (FileExists(Volume->RootDir, FileName)) {
 //     Volume->OSType = OSTYPE_LIN;
      Volume->BootType = BOOTING_BY_EFI;
      if (!gSettings.HVHideAllRedHat)
      Entry = AddLoaderEntry(FileName, L"RedHat EFI boot menu", Volume, OSTYPE_LIN);
//      continue;
    }

    // check for Redhat boot loader/menu
    StrCpy(FileName, L"\\EFI\\RedHat\\grubx64.efi");
    if (FileExists(Volume->RootDir, FileName)) {
//      Volume->OSType = OSTYPE_LIN;
      Volume->BootType = BOOTING_BY_EFI;
      if (!gSettings.HVHideAllRedHat)
      Entry = AddLoaderEntry(FileName, L"RedHat EFI boot menu", Volume, OSTYPE_LIN);
 //     continue;
    }
    
    // check for Ubuntu boot loader/menu
#if defined(MDE_CPU_X64)    
    StrCpy(FileName, L"\\EFI\\Ubuntu\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\Ubuntu\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
//      Volume->OSType = OSTYPE_LIN;
      Volume->BootType = BOOTING_BY_EFI;
      if (!gSettings.HVHideAllUbuntu)
      Entry = AddLoaderEntry(FileName, L"Ubuntu EFI boot menu", Volume, OSTYPE_LIN);
//      continue;
    }
    
    // check for Linux Mint boot loader/menu
#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\Linuxmint\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\Linuxmint\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      //      Volume->OSType = OSTYPE_LIN;
      Volume->BootType = BOOTING_BY_EFI;
      if (!gSettings.HVHideAllLinuxMint)
        Entry = AddLoaderEntry(FileName, L"Linux Mint EFI boot menu", Volume, OSTYPE_LIN);
      //      continue;
    }
    
    // check for OpenSuse boot loader/menu
    StrCpy(FileName, L"\\EFI\\SuSe\\elilo.efi");
    if (FileExists(Volume->RootDir, FileName)) {
//      Volume->OSType = OSTYPE_LIN;
      Volume->BootType = BOOTING_BY_EFI;
      if (!gSettings.HVHideAllSuSe)
      Entry = AddLoaderEntry(FileName, L"OpenSuse EFI boot menu", Volume, OSTYPE_LIN);
//      continue;
    }
#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\opensuse\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\opensuse\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
//      Volume->OSType = OSTYPE_LIN;
      Volume->BootType = BOOTING_BY_EFI;
      if (!gSettings.HVHideAllSuSe)
      Entry = AddLoaderEntry(FileName, L"OpenSuse EFI boot menu", Volume, OSTYPE_LIN);
//      continue;
    }
    
    // check for Clover on EFI partition
//move to ScanTools    
/*    Status = gBS->HandleProtocol (Volume->DeviceHandle, &gEfiPartTypeSystemPartGuid, &Interface);
    if (Status == EFI_SUCCESS && !gSettings.HVHideAllUEFI) {
#if defined(MDE_CPU_X64)
      StrCpy(FileName, L"\\EFI\\BOOT\\CLOVERX64.EFI");
#else
      StrCpy(FileName, L"\\EFI\\BOOT\\CLOVERIA32.EFI");
#endif
      if (FileExists(Volume->RootDir, FileName)) {
        Volume->BootType = BOOTING_BY_EFI;
        AddCloverEntry(FileName, L"Clover Boot Options", Volume);
      }
    }
*/    
    //UEFI bootloader XXX
#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\BOOT\\BOOTX64.efi");
#else      
    StrCpy(FileName, L"\\EFI\\BOOT\\BOOTIA32.efi");
#endif
    DBG("search for UEFI\n");
    if (FileExists(Volume->RootDir, FileName)) {
//      Volume->OSType = OSTYPE_VAR;
      Volume->BootType = BOOTING_BY_EFI;
      if (!gSettings.HVHideAllUEFI)
      AddLoaderEntry(FileName, L"UEFI boot menu", Volume, OSTYPE_VAR);
//      continue;
    }
  }
}


#define MAX_DISCOVERED_PATHS (16)
//#define PREBOOT_LOG L"EFI\\misc\\preboot.log"

static VOID StartLegacy(IN LEGACY_ENTRY *Entry)
{
    EFI_STATUS          Status = EFI_UNSUPPORTED;
    EG_IMAGE            *BootLogoImage;
//    UINTN               ErrorInStep = 0;
//    EFI_DEVICE_PATH     *DiscoveredPathList[MAX_DISCOVERED_PATHS];

    SetStartupDiskVolume(Entry->Volume, NULL);
  
    egClearScreen(&DarkBackgroundPixel);
    BeginExternalScreen(TRUE, L"Booting Legacy OS");
    
    BootLogoImage = LoadOSIcon(Entry->Volume->OSIconName, L"legacy", TRUE);
    if (BootLogoImage != NULL)
        BltImageAlpha(BootLogoImage,
                      (UGAWidth  - BootLogoImage->Width) >> 1,
                      (UGAHeight - BootLogoImage->Height) >> 1,
                      &StdBackgroundPixel, 16);
  
/*    Status = ExtractLegacyLoaderPaths(DiscoveredPathList, MAX_DISCOVERED_PATHS, LegacyLoaderList);
    if (!EFI_ERROR(Status)) {
      Status = StartEFIImageList(DiscoveredPathList, Entry->LoadOptions, NULL, L"legacy loader", &ErrorInStep, NULL);
    } */
 //   if (EFI_ERROR(Status)) {
      //try my LegacyBoot
      switch (Entry->Volume->BootType) {
        case BOOTING_BY_CD:
          Status = bootElTorito(Entry->Volume);
          break;
        case BOOTING_BY_MBR:
          Status = bootMBR(Entry->Volume);
          break;
        case BOOTING_BY_PBR:
          if (StrCmp(gSettings.LegacyBoot, L"LegacyBiosDefault") == 0) {
            Status = bootLegacyBiosDefault(Entry->Volume);
          } else if (StrCmp(gSettings.LegacyBoot, L"PBRtest") == 0) {
            Status = bootPBRtest(Entry->Volume);
          } else {
            // default
            Status = bootPBR(Entry->Volume);
          }
          break;
        default:
          break;
      }
      CheckError(Status, L"while LegacyBoot");
//      if (0 && Entry->Volume->IsMbrPartition && !Entry->Volume->HasBootCode)
//         ActivateMbrPartition(Entry->Volume->WholeDiskBlockIO, Entry->Volume->MbrPartitionIndex);

/*        if (ErrorInStep == 1)
            Print(L"\nPlease make sure that you have the latest firmware update installed.\n");
        else if (ErrorInStep == 3)
            Print(L"\nThe firmware refused to boot from the selected volume. Note that external\n"
                  L"hard drives are not well-supported by Apple's firmware for legacy OS booting.\n");
 */
//    }
    FinishExternalScreen();
}

static LEGACY_ENTRY * AddLegacyEntry(IN CHAR16 *LoaderTitle, IN REFIT_VOLUME *Volume)
{
  LEGACY_ENTRY            *Entry, *SubEntry;
  REFIT_MENU_SCREEN       *SubScreen;
  CHAR16                  *VolDesc;
  CHAR16                  ShortcutLetter = 0;
  
  if (LoaderTitle == NULL) {
    if (Volume->OSName != NULL) {
      LoaderTitle = Volume->OSName;
      if (LoaderTitle[0] == 'W' || LoaderTitle[0] == 'L')
        ShortcutLetter = LoaderTitle[0];
    } else
      LoaderTitle = L"Legacy OS";
  }
  if (Volume->VolName != NULL)
    VolDesc = Volume->VolName;
  else
    VolDesc = (Volume->DiskKind == DISK_KIND_OPTICAL) ? L"CD" : L"HD";
  
  // prepare the menu entry
  Entry = AllocateZeroPool(sizeof(LEGACY_ENTRY));
  Entry->me.Title        = PoolPrint(L"Boot %s from %s", LoaderTitle, VolDesc);
  Entry->me.Tag          = TAG_LEGACY;
  Entry->me.Row          = 0;
  Entry->me.ShortcutLetter = ShortcutLetter;
  Entry->me.Image        = LoadOSIcon(Volume->OSIconName, L"legacy", FALSE);
  //  DBG("HideBadges=%d Volume=%s\n", GlobalConfig.HideBadges, Volume->VolName);
  //  DBG("Title=%s OSName=%s OSIconName=%s\n", LoaderTitle, Volume->OSName, Volume->OSIconName);
  
  //actions
  Entry->me.AtClick = ActionSelect;
  Entry->me.AtDoubleClick = ActionEnter;
  Entry->me.AtRightClick = ActionDetails;
  
  if ((GlobalConfig.HideBadges == HDBADGES_NONE) ||
      (GlobalConfig.HideBadges == HDBADGES_INT &&
       Volume->DiskKind != DISK_KIND_INTERNAL)) { //hide internal
    Entry->me.BadgeImage   = egCopyScaledImage(Volume->OSImage, 8);
    //    Entry->me.BadgeImage   = egLoadIcon(ThemeDir, PoolPrint(L"icons\\os_%s.icns", Volume->OSIconName), 32);
  } else if (GlobalConfig.HideBadges == HDBADGES_SWAP) {
    Entry->me.BadgeImage   =  egCopyScaledImage(Volume->DriveImage, 4);
  }
  Entry->Volume          = Volume;
  Entry->LoadOptions     = (Volume->DiskKind == DISK_KIND_OPTICAL) ? L"CD" :
  ((Volume->DiskKind == DISK_KIND_EXTERNAL) ? L"USB" : L"HD");
  
  // create the submenu
  SubScreen = AllocateZeroPool(sizeof(REFIT_MENU_SCREEN));
  SubScreen->Title = PoolPrint(L"Boot Options for %s on %s", LoaderTitle, VolDesc);
  SubScreen->TitleImage = Entry->me.Image;
  SubScreen->AnimeRun = GetAnime(SubScreen);;
  
  // default entry
  SubEntry = AllocateZeroPool(sizeof(LEGACY_ENTRY));
  SubEntry->me.Title        = PoolPrint(L"Boot %s", LoaderTitle);
  SubEntry->me.Tag          = TAG_LEGACY;
  SubEntry->Volume          = Entry->Volume;
  SubEntry->LoadOptions     = Entry->LoadOptions;
  SubEntry->me.AtClick      = ActionEnter;
  AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
  
  AddMenuEntry(SubScreen, &MenuEntryReturn);
  Entry->me.SubScreen = SubScreen;
  AddMenuEntry(&MainMenu, (REFIT_MENU_ENTRY *)Entry);
  return Entry;
}

static VOID ScanLegacy(VOID)
{
    UINTN                   VolumeIndex, VolumeIndex2;
    BOOLEAN                 ShowVolume, HideIfOthersFound;
    REFIT_VOLUME            *Volume;
    
    // Print(L"Scanning for legacy boot volumes... VolumesCount = %d\n", VolumesCount);
    
    for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
        Volume = Volumes[VolumeIndex];
#if 0 // REFIT_DEBUG > 0
        DBG(" %d %s\n  %d %d %s %d %s\n",
              VolumeIndex, DevicePathToStr(Volume->DevicePath),
              Volume->DiskKind, Volume->MbrPartitionIndex,
              Volume->IsAppleLegacy ? L"AL" : L"--", Volume->HasBootCode,
              Volume->VolName ? Volume->VolName : L"(no name)");
#endif
        
        // skip volume if its kind is configured as disabled
        if ((Volume->DiskKind == DISK_KIND_OPTICAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_OPTICAL)) ||
            (Volume->DiskKind == DISK_KIND_EXTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_EXTERNAL)) ||
            (Volume->DiskKind == DISK_KIND_INTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_INTERNAL)))
          continue;
        
        if ((Volume->BootType == BOOTING_BY_EFI) ||
            (Volume->BootType == BOOTING_BY_BOOTEFI)) {
          continue; //this is not legacy!!!
        }
      
        ShowVolume = FALSE;
        HideIfOthersFound = FALSE;
        if (Volume->IsAppleLegacy) {
            ShowVolume = TRUE;
            HideIfOthersFound = TRUE;
        } else if (Volume->HasBootCode) {
            ShowVolume = TRUE;
            //DBG("Volume %d will be shown BlockIo=%x WholeIo=%x\n",
            //  VolumeIndex, Volume->BlockIO, Volume->WholeDiskBlockIO);
            if ((Volume->WholeDiskBlockIO == 0) &&
                Volume->BlockIOOffset == 0 /* &&
                Volume->OSName == NULL */)
                // this is a whole disk (MBR) entry; hide if we have entries for partitions
                HideIfOthersFound = TRUE;
        }
        if (HideIfOthersFound) {          
          // check for other bootable entries on the same disk
          //if PBR exists then Hide MBR
            for (VolumeIndex2 = 0; VolumeIndex2 < VolumesCount; VolumeIndex2++) {
                if (VolumeIndex2 != VolumeIndex && 
                    Volumes[VolumeIndex2]->HasBootCode && 
                    Volumes[VolumeIndex2]->WholeDiskBlockIO == Volume->BlockIO){
                    ShowVolume = FALSE;
     //             DBG("PBR volume at index %d\n", VolumeIndex2);
                  break;
                }
            }
          if (!ShowVolume) {
            DBG("hide volume %d\n", VolumeIndex);
          }
        }
        
        if (ShowVolume && (Volume->OSType != OSTYPE_HIDE)){
            AddLegacyEntry(NULL, Volume);
            DBG("added legacy entry %d OSType=%d Name=%s Icon=%s\n",
                VolumeIndex, Volume->OSType, Volume->VolName? Volume->VolName: L"<null>", Volume->OSIconName);
        }
    }
}

//
// pre-boot tool functions
//

static VOID StartTool(IN LOADER_ENTRY *Entry)
{
  egClearScreen(&DarkBackgroundPixel);
    BeginExternalScreen(Entry->UseGraphicsMode, Entry->me.Title + 6);  // assumes "Start <title>" as assigned below
    StartEFIImage(Entry->DevicePath, Entry->LoadOptions, Basename(Entry->LoaderPath),
                  Basename(Entry->LoaderPath), NULL, NULL);
    FinishExternalScreen();
//  ReinitSelfLib();
}

static LOADER_ENTRY * AddToolEntry(IN CHAR16 *LoaderPath, IN CHAR16 *LoaderTitle,
                                   IN EG_IMAGE *Image,
                                   IN CHAR16 ShortcutLetter, IN BOOLEAN UseGraphicsMode)
{
  LOADER_ENTRY *Entry;
  
  Entry = AllocateZeroPool(sizeof(LOADER_ENTRY));
  
  Entry->me.Title = PoolPrint(L"Start %s", LoaderTitle);
  Entry->me.Tag = TAG_TOOL;
  Entry->me.Row = 1;
  Entry->me.ShortcutLetter = ShortcutLetter;
  Entry->me.Image = Image;
  Entry->LoaderPath = EfiStrDuplicate(LoaderPath);
  Entry->DevicePath = FileDevicePath(SelfDeviceHandle, Entry->LoaderPath);
  Entry->UseGraphicsMode = UseGraphicsMode;
  //actions
  Entry->me.AtClick = ActionSelect;
  Entry->me.AtDoubleClick = ActionEnter;
  Entry->me.AtRightClick = ActionHelp;
  
  AddMenuEntry(&MainMenu, (REFIT_MENU_ENTRY *)Entry);
  return Entry;
}

static VOID ScanTool(VOID)
{
  EFI_STATUS              Status;
  CHAR16                  FileName[256];
  LOADER_ENTRY            *Entry;
  UINTN                   VolumeIndex;
  REFIT_VOLUME            *Volume;
  VOID                    *Interface;
  
  if (GlobalConfig.DisableFlags & DISABLE_FLAG_TOOLS)
    return;
  
  if (!gFirmwareClover) {
    for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
      Volume = Volumes[VolumeIndex];
      if (!Volume->RootDir || !Volume->DeviceHandle) {
        continue;
      }
      
      Status = gBS->HandleProtocol (Volume->DeviceHandle, &gEfiPartTypeSystemPartGuid, &Interface);
      if (Status == EFI_SUCCESS && !gSettings.HVHideAllUEFI) {
        DBG("Checking EFI partition Volume %d for Clover\n", VolumeIndex);
        
#if defined(MDE_CPU_X64)
        StrCpy(FileName, L"\\EFI\\BOOT\\CLOVERX64.EFI");
#else
        StrCpy(FileName, L"\\EFI\\BOOT\\CLOVERIA32.EFI");
#endif
        
        // OSX adds label "EFI" to EFI volumes and some UEFIs see that
        // as a file. This file then blocks access to the /EFI directory.
        // We will delete /EFI file here and leave only /EFI directory.
        if (DeleteFile(Volume->RootDir, L"EFI")) {
          DBG(" Deleted /EFI label\n");
        }
        
        if (FileExists(Volume->RootDir, FileName)) {
          DBG(" Found Clover\n");
          Volume->BootType = BOOTING_BY_EFI;
          AddCloverEntry(FileName, L"Clover Boot Options", Volume);
        }
      }
    }
  }
  
  //    Print(L"Scanning for tools...\n");
  
  // look for the EFI shell
  if (!(GlobalConfig.DisableFlags & DISABLE_FLAG_SHELL)) {
#if defined(MDE_CPU_IA32)
    StrCpy(FileName, L"\\EFI\\tools\\Shell32.efi");
    if (FileExists(SelfRootDir, FileName)) {
      Entry = AddToolEntry(FileName, L"EFI Shell 32", BuiltinIcon(BUILTIN_ICON_TOOL_SHELL), 'S', FALSE);
      DBG("found tools\\Shell32.efi\n");
    }
#elif defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\tools\\Shell64.efi");
    if (FileExists(SelfRootDir, FileName)) {
      Entry = AddToolEntry(FileName, L"EFI Shell 64", BuiltinIcon(BUILTIN_ICON_TOOL_SHELL), 'S', FALSE);
      DBG("found tools\\Shell64.efi\n");
    }
#else //what else? ARM?
    UnicodeSPrint(FileName, 512, L"\\EFI\\tools\\shell.efi");
    if (FileExists(SelfRootDir, FileName)) {
      Entry = AddToolEntry(FileName, L"EFI Shell", BuiltinIcon(BUILTIN_ICON_TOOL_SHELL), 'S', FALSE);
      DBG("found apps\\shell.efi\n");
    }
#endif
  }
  
  
  
  // look for the GPT/MBR sync tool
  /*    StrCpy(FileName, L"\\efi\\tools\\gptsync.efi");
   if (FileExists(SelfRootDir, FileName)) {
   Entry = AddToolEntry(FileName, L"Partitioning Tool", BuiltinIcon(BUILTIN_ICON_TOOL_PART), 'P', FALSE);
   }*/
  /*    
   // look for rescue Linux
   StrCpy(FileName, L"\\efi\\rescue\\elilo.efi");
   if (SelfVolume != NULL && FileExists(SelfRootDir, FileName)) {
   Entry = AddToolEntry(FileName, L"Rescue Linux", BuiltinIcon(BUILTIN_ICON_TOOL_RESCUE), 0, FALSE);
   
   if (UGAWidth == 1440 && UGAHeight == 900)
   Entry->LoadOptions = L"-d 0 i17";
   else if (UGAWidth == 1680 && UGAHeight == 1050)
   Entry->LoadOptions = L"-d 0 i20";
   else
   Entry->LoadOptions = L"-d 0 mini";
   }
   */
}

//
// pre-boot driver functions
//

static VOID ScanDriverDir(IN CHAR16 *Path, OUT EFI_HANDLE **DriversToConnect, OUT UINTN *DriversToConnectNum)
{
  EFI_STATUS              Status;
  REFIT_DIR_ITER          DirIter;
  EFI_FILE_INFO           *DirEntry;
  CHAR16                  FileName[256];
  EFI_HANDLE              DriverHandle;
  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding;
  UINTN                   DriversArrSize;
  UINTN                   DriversArrNum;
  EFI_HANDLE              *DriversArr;
  
  
  DriversArrSize = 0;
  DriversArrNum = 0;
  DriversArr = NULL;
  
  // look through contents of the directory
  DirIterOpen(SelfRootDir, Path, &DirIter);
  while (DirIterNext(&DirIter, 2, L"*.EFI", &DirEntry)) {
    if (DirEntry->FileName[0] == '.')
      continue;   // skip this
    
    UnicodeSPrint(FileName, 512, L"%s\\%s", Path, DirEntry->FileName);
    Status = StartEFIImage(FileDevicePath(SelfLoadedImage->DeviceHandle, FileName),
                           L"", DirEntry->FileName, DirEntry->FileName, NULL, &DriverHandle);
    if (EFI_ERROR(Status)) {
      continue;
    }
    if (StrStr(FileName, L"EmuVariable") != NULL) {
      gDriversFlags.EmuVariableLoaded = TRUE;
      gFirmwarePhoenix = TRUE;
    } else if (StrStr(FileName, L"Video") != NULL) {
      gDriversFlags.VideoLoaded = TRUE;
    } else if (StrStr(FileName, L"Partition") != NULL) {
      gDriversFlags.PartitionLoaded = TRUE;
    }
    if (DriverHandle != NULL && DriversToConnectNum != NULL && DriversToConnect != NULL) {
      // driver loaded - check for EFI_DRIVER_BINDING_PROTOCOL
      Status = gBS->HandleProtocol(DriverHandle, &gEfiDriverBindingProtocolGuid, (VOID **) &DriverBinding);
      if (!EFI_ERROR(Status) && DriverBinding != NULL) {
        DBG(" - driver needs connecting\n");
        // standard UEFI driver - we would reconnect after loading - add to array
        if (DriversArrSize == 0) {
          // new array
          DriversArrSize = 16;
          DriversArr = AllocateZeroPool(sizeof(EFI_HANDLE) * DriversArrSize);
        } else if (DriversArrNum + 1 == DriversArrSize) {
          // extend array
          DriversArr = ReallocatePool(DriversArrSize, DriversArrSize + 16, DriversArr);
          DriversArrSize += 16;
        }
        DriversArr[DriversArrNum] = DriverHandle;
        DriversArrNum++;
        // we'll make array terminated
        DriversArr[DriversArrNum] = NULL;
      }
    }
  }
  Status = DirIterClose(&DirIter);
  if (Status != EFI_NOT_FOUND) {
    UnicodeSPrint(FileName, 512, L"while scanning the %s directory", Path);
    CheckError(Status, FileName);
  }
  
  if (DriversToConnectNum != NULL && DriversToConnect != NULL) {
    *DriversToConnectNum = DriversArrNum;
    *DriversToConnect = DriversArr;
  }
}


VOID DisconnectSomeDevices(VOID)
{
  EFI_STATUS              Status;
  UINTN                   HandleCount = 0;
  UINTN                   Index;
  EFI_HANDLE              *Handles = NULL;
	EFI_BLOCK_IO_PROTOCOL   *BlockIo	= NULL;
	EFI_PCI_IO_PROTOCOL     *PciIo	= NULL;
	PCI_TYPE00              Pci;
  
  if (gDriversFlags.PartitionLoaded) {
    DBG("Partition driver loaded: ");
    // get all BlockIo handles
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &HandleCount, &Handles);
    if (Status == EFI_SUCCESS) {
      for (Index = 0; Index < HandleCount; Index++) {
        Status = gBS->HandleProtocol(Handles[Index], &gEfiBlockIoProtocolGuid, (VOID **) &BlockIo);
        if (EFI_ERROR(Status)) {
          continue;
        }
        if (BlockIo->Media->BlockSize == 2048) {
          // disconnect CD driver
          Status = gBS->DisconnectController(Handles[Index], NULL, NULL);
          DBG("CD disconnect %r", Status);
        }
      }
      FreePool(Handles);
    }
    DBG("\n");
  }
  
  if (gDriversFlags.VideoLoaded) {
    DBG("Video driver loaded: ");
    // get all PciIo handles
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiPciIoProtocolGuid, NULL, &HandleCount, &Handles);
    if (Status == EFI_SUCCESS) {
      for (Index = 0; Index < HandleCount; Index++) {
        Status = gBS->HandleProtocol(Handles[Index], &gEfiPciIoProtocolGuid, (VOID **) &PciIo);
        if (EFI_ERROR(Status)) {
          continue;
        }
        Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0, sizeof (Pci) / sizeof (UINT32), &Pci);
        if (!EFI_ERROR (Status))
        {
          if(IS_PCI_VGA(&Pci) == TRUE)
          {
            // disconnect VGA
            Status = gBS->DisconnectController(Handles[Index], NULL, NULL);
            DBG("disconnect %r", Status);
          }
        }
      }
      FreePool(Handles);
    }
    DBG("\n");
  }
}


static VOID LoadDrivers(VOID)
{
  EFI_HANDLE  *DriversToConnect = NULL;
  UINTN       DriversToConnectNum = 0;
  
    // load drivers from /efi/drivers
#if defined(MDE_CPU_X64)
  if (gFirmwareClover) {
    ScanDriverDir(L"\\EFI\\drivers64", &DriversToConnect, &DriversToConnectNum);
  } else
    ScanDriverDir(L"\\EFI\\drivers64UEFI", &DriversToConnect, &DriversToConnectNum);
#else
  ScanDriverDir(L"\\EFI\\drivers32", &DriversToConnect, &DriversToConnectNum);
#endif
  
  if (DriversToConnectNum > 0) {
    DBGT("%d drivers needs connecting ...\n", DriversToConnectNum);
    // note: our platform driver protocol
    // will use DriversToConnect - do not release it
    RegisterDriversToHighestPriority(DriversToConnect);
    DisconnectSomeDevices();
    BdsLibConnectAllDriversToAllControllers();
  }
}


INTN FindDefaultEntry(VOID)
{
  INTN                Index;
  REFIT_VOLUME        *Volume;
  LOADER_ENTRY        *Entry;

  
  DBG("FindDefaultEntry ...\n");
  
  //
  // try to detect volume set by Startup Disk or previous Clover selection
  //
  Index = FindStartupDiskVolume(&MainMenu);
  if (Index >= 0) {
    DBG("Boot redirected to Entry %d. '%s'\n", Index, MainMenu.Entries[Index]->Title);
    return Index;
  }
  
  //
  // if not found, then try DefaultBoot from config.plist
  // search volume with name == gSettings.DefaultBoot
  //
  if (gSettings.DefaultBoot != NULL && gSettings.DefaultBoot[0] != L'\0') {
    
    DBG("Searching config.plist DefaultBoot ...");
    for (Index = 0; ((Index < (INTN)MainMenu.EntryCount) && (MainMenu.Entries[Index]->Row == 0)); Index++) {
      
      Entry = (LOADER_ENTRY*)MainMenu.Entries[Index];
      if (!Entry->Volume) {
        continue;
      }
      
      Volume = Entry->Volume;
      if (StrCmp(Volume->VolName, gSettings.DefaultBoot) != 0) {
        continue;
      }
      
      DBG(" found\nBoot redirected to Entry %d. '%s', Volume '%s'\n", Index, Entry->me.Title, Volume->VolName);
      return Index;
    }
    
  }
  
  DBG("Default boot entry not found\n");
  return -1;
}

//
// main entry point
//

EFI_STATUS
EFIAPI
RefitMain (IN EFI_HANDLE           ImageHandle,
           IN EFI_SYSTEM_TABLE     *SystemTable)
{
  EFI_STATUS Status;
  BOOLEAN           MainLoopRunning = TRUE;
  BOOLEAN           ReinitDesktop = TRUE;
  BOOLEAN           AfterTool = FALSE;
  REFIT_MENU_ENTRY  *ChosenEntry = NULL;
  REFIT_MENU_ENTRY  *DefaultEntry = NULL;
  REFIT_MENU_ENTRY  *OptionEntry = NULL;
  INTN              DefaultIndex;
  UINTN             MenuExit;
  UINTN             Size, i;
  UINT64            TscDiv;
  UINT64            TscRemainder = 0;
  LOADER_ENTRY      *LoaderEntry;
  UINT64            t0, t1;
  
  // CHAR16            *InputBuffer; //, *Y;
  //  EFI_INPUT_KEY Key;
  
  // bootstrap
  //    InitializeLib(ImageHandle, SystemTable);
	gST				= SystemTable;
	gImageHandle	= ImageHandle;
	gBS				= SystemTable->BootServices;
	gRS				= SystemTable->RuntimeServices;
  
	Status = EfiGetSystemConfigurationTable (&gEfiDxeServicesTableGuid, (VOID **) &gDS);
	
  // firmware detection
  gFirmwareClover = StrCmp(gST->FirmwareVendor, L"CLOVER") == 0;
  gFirmwarePhoenix = StrStr(gST->FirmwareVendor, L"Phoenix") != NULL;
  //	DBG("Running on Firmware %s, it is Clover?%a\n", gST->FirmwareVendor, gFirmwareClover?"Yes":"No");
  
  InitializeConsoleSim();
	InitBooterLog();
  DBG(" \nStarting rEFIt rev %s on %s EFI\n", FIRMWARE_REVISION, gST->FirmwareVendor);
  //  InitScreen();
  /*    
   DBG("Test arithmetics\n");
   UINT64 X = 123000123;
   UINT32 Y = 453000;
   UINT32 Z = DivU64x32(X, Y);
   DBG("X=%ld Y=%d Z=%d again=%d\n", X, Y, Z, DivU64x32(X, Y));
   X = MultU64x32(Z, Y);
   DBG("Z*Y=%ld again=%ld\n", X, MultU64x32(Z, Y));
   PauseForKey(L"Test complete");
   */    
  Status = InitRefitLib(gImageHandle);
  if (EFI_ERROR(Status))
    return Status;
  
  InitAnime();
  InitializeUnicodeCollationProtocol();
  
  // read GUI configuration
  ReadConfig();
  
  // init screen and dump video modes to log
  InitScreen(!gFirmwareClover); // ? FALSE : TRUE);
  
  // disable EFI watchdog timer
  gBS->SetWatchdogTimer(0x0000, 0x0000, 0x0000, NULL);
  
  ThemePath = PoolPrint(L"EFI\\BOOT\\themes\\%s", GlobalConfig.Theme);
  Status = SelfRootDir->Open(SelfRootDir, &ThemeDir, ThemePath, EFI_FILE_MODE_READ, 0);
  // if (EFI_ERROR (Status)) { return Status; }
  DBG("Theme: %s Path: %s\n", GlobalConfig.Theme, ThemePath);
  MainMenu.TimeoutSeconds = GlobalConfig.Timeout >= 0 ? GlobalConfig.Timeout : 0;
  
  PrepatchSmbios();

  // init debug time - must be after PrepatchSmbios()
  DbgTimeInit();
#ifdef REVISION_STR
  MsgLog(REVISION_STR); 
#endif
  DBG("  running on %a\n", gSettings.OEMProduct);
  DBG("... with board %a\n", gSettings.OEMBoard);
  //replace / with _
  Size = iStrLen(gSettings.OEMProduct, 64);
  for (i=0; i<Size; i++) {
    if (gSettings.OEMProduct[i] == 0x2F) {
      gSettings.OEMProduct[i] = 0x5F;
    }
  }
  Size = iStrLen(gSettings.OEMBoard, 64);
  for (i=0; i<Size; i++) {
    if (gSettings.OEMBoard[i] == 0x2F) {
      gSettings.OEMBoard[i] = 0x5F;
    }
  }
  
  
  if (FileExists(SelfRootDir, PoolPrint(L"EFI\\OEM\\%a\\config.plist", gSettings.OEMProduct))) {
    OEMPath = PoolPrint(L"EFI\\OEM\\%a", gSettings.OEMProduct);
  } else if (FileExists(SelfRootDir, PoolPrint(L"EFI\\OEM\\%a\\config.plist", gSettings.OEMBoard))) {
    OEMPath = PoolPrint(L"EFI\\OEM\\%a", gSettings.OEMBoard);
  } else {
    OEMPath = L"EFI";
  }
  
  // further bootstrap (now with config available)
  //  SetupScreen();
  
  DBGT("LoadDrivers() start\n");
  LoadDrivers();
  DBGT("LoadDrivers() end\n");
  
  if (gDriversFlags.VideoLoaded) {
    // reinit screen and dump video modes to log
    InitScreen(FALSE);
  }
  
  //Now we have to reinit handles
  Status = ReinitSelfLib();
  if (EFI_ERROR(Status)){
    DebugLog(2, " %r", Status);
    PauseForKey(L"Error reinit refit\n");
    return Status;
  }
//        DBG("reinit OK\n");
  ZeroMem((VOID*)&gSettings, sizeof(SETTINGS_DATA));
  ZeroMem((VOID*)&gGraphics[0], sizeof(GFX_PROPERTIES) * 4);

  GuiEventsInitialize();
  DBG("GuiEventsInitialize OK\n");
  t0 = AsmReadTsc();
  gBS->Stall(100000); //100ms
  t1 = AsmReadTsc();
  gCPUStructure.TSCCalibr = MultU64x32((t1 - t0), 10); //ticks for 1second
  
  
  GetCPUProperties();
  if (!gSettings.EnabledCores) {
    gSettings.EnabledCores = gCPUStructure.Cores;
  }
  DBG("GetCPUProperties OK\n");
  GetDevices();
 //     DBG("GetDevices OK\n");
  DBGT("ScanSPD() start\n");
  ScanSPD();
  DBGT("ScanSPD() end\n");
 //       DBG("ScanSPD OK\n");
  SetPrivateVarProto();
//        DBG("SetPrivateVarProto OK\n");
  GetDefaultSettings();
  DBG("Calibrated TSC frequency =%ld =%ldMHz\n", gCPUStructure.TSCCalibr, DivU64x32(gCPUStructure.TSCCalibr, Mega));
  DBG("CPU calculated TSC frequency =%ld\n", gCPUStructure.TSCFrequency);
  if (gCPUStructure.TSCFrequency > gCPUStructure.TSCCalibr) {
    TscDiv = DivU64x64Remainder(gCPUStructure.TSCFrequency, gCPUStructure.TSCCalibr, &TscRemainder);
  } else {
    TscDiv = DivU64x64Remainder(gCPUStructure.TSCCalibr, gCPUStructure.TSCFrequency, &TscRemainder);
  }

  if ((TscRemainder > 400 * Mega) || (TscDiv > 1))
  {
    DBG("There is a problem with TSC detection and calibration! Assume calibrated one\n");
    gCPUStructure.TSCFrequency = gCPUStructure.TSCCalibr;
  }
  
  //Second step. Load config.plist into gSettings	
	Status = GetUserSettings(SelfRootDir);  
 //       DBG("GetUserSettings OK\n");
  
  //test font
  PrepareFont();
 //     DBG("PrepareFont OK\n");
  FillInputs();
  
  if (!gFirmwareClover && !gFirmwarePhoenix &&
      GlobalConfig.Timeout == 0 && !ReadAllKeyStrokes()) {
    // UEFI boot: get gEfiBootDeviceGuid from NVRAM.
    // if present, ScanVolumes() will skip scanning other volumes
    // in the first run.
    // this speeds up loading of default OSX volume.
    GetEfiBootDeviceFromNvram();
//    DBGT("GetEfiBootDeviceFromNvram()\n");
  }
  
  do {
//     PauseForKey(L"Enter main cycle");
//    DBGT("Enter main cycle\n");
    AfterTool = FALSE;
    MainMenu.EntryCount = 0;
    ScanVolumes();
 //   DBGT("ScanVolumes()\n");
    
    // as soon as we have Volumes, find lates nvram.plist and copy it to RT vars
    if (gFirmwareClover || gFirmwarePhoenix) {
      PutNvramPlistToRtVars();
    }
    
    // scan for loaders and tools, add then to the menu
    if (!GlobalConfig.NoLegacy && GlobalConfig.LegacyFirst && !gSettings.HVHideAllLegacy){
      DBG("scan legacy first\n");
      ScanLegacy();
//      DBGT("ScanLegacy()\n");
    }
    ScanLoader();
//    DBGT("ScanLoader()\n");
//          DBG("ScanLoader OK\n");
    if (!GlobalConfig.NoLegacy && !GlobalConfig.LegacyFirst && !gSettings.HVHideAllLegacy){
//      DBG("scan legacy second\n");
      ScanLegacy();
      DBGT("ScanLegacy()\n");
    }
//    DBG("ScanLegacy OK\n");
    // fixed other menu entries
//               DBG("FillInputs OK\n");
    if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_FUNCS)) {
      MenuEntryAbout.Image = BuiltinIcon(BUILTIN_ICON_FUNC_ABOUT);
      AddMenuEntry(&MainMenu, &MenuEntryAbout);
    }
    
    if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_FUNCS)) {
      MenuEntryOptions.Image = BuiltinIcon(BUILTIN_ICON_FUNC_OPTIONS);
      AddMenuEntry(&MainMenu, &MenuEntryOptions);
    }  

    if (!(GlobalConfig.DisableFlags & DISABLE_FLAG_TOOLS)) {
      //            DBG("scan tools\n");
      ScanTool();
      DBGT("ScanTool()\n");
    }
//    DBG("ScanTool OK\n");
    
    if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_FUNCS) || MainMenu.EntryCount == 0) {
      MenuEntryReset.Image = BuiltinIcon(BUILTIN_ICON_FUNC_RESET);
      //    DBG("Reset.Image->Width=%d\n", MenuEntryReset.Image->Width);
      AddMenuEntry(&MainMenu, &MenuEntryReset);
      MenuEntryShutdown.Image = BuiltinIcon(BUILTIN_ICON_FUNC_SHUTDOWN);
      //    DBG("Shutdown.Image->Width=%d\n", MenuEntryShutdown.Image->Width);
      AddMenuEntry(&MainMenu, &MenuEntryShutdown);
    }
        
    // wait for user ACK when there were errors
    FinishTextScreen(FALSE);
    DBGT("FinishTextScreen()\n");
    
    DefaultIndex = FindDefaultEntry();
    DBGT("FindDefaultEntry()\n");
    //  DBG("DefaultIndex=%d and MainMenu.EntryCount=%d\n", DefaultIndex, MainMenu.EntryCount);
    if ((DefaultIndex >= 0) && (DefaultIndex < (INTN)MainMenu.EntryCount)) {
      DefaultEntry = MainMenu.Entries[DefaultIndex];
    } else {
      DefaultEntry = NULL;
    }
    
    MainMenu.AnimeRun = GetAnime(&MainMenu);
    // PauseForKey(L"Enter main loop");
    MainLoopRunning = TRUE;
    gEvent = 0; //clear to cancel loop
    while (MainLoopRunning) {
      if (GlobalConfig.Timeout == 0 && DefaultEntry != NULL && !ReadAllKeyStrokes()) {
        // go strait to DefaultVolume loading
        MenuExit = MENU_EXIT_TIMEOUT;
      } else {
        //    DBG("Enter main loop\n");
 //       DBGT("RunMainMenu() start\n");
        MenuExit = RunMainMenu(&MainMenu, DefaultIndex, &ChosenEntry);
 //       DBGT("RunMainMenu() end\n");
      }
      // disable default boot - have sense only in the first run
      GlobalConfig.Timeout = -1;
      
      //    DBG("out from menu\n");
      if ((DefaultEntry != NULL) && (MenuExit == MENU_EXIT_TIMEOUT)) {
        //      DBG("boot by timeout\n");
        if (DefaultEntry->Tag == TAG_LOADER) {
          StartLoader((LOADER_ENTRY *)DefaultEntry);
        } else if (DefaultEntry->Tag == TAG_LEGACY){
          StartLegacy((LEGACY_ENTRY *)DefaultEntry);
        }
        // if something goes wrong - break main loop to reinit volumes
        break;
      }
      
      if (MenuExit == MENU_EXIT_OPTIONS){
        OptionsMenu(&OptionEntry);
        //ApplyInputs();
        continue;
      }
      
      if (MenuExit == MENU_EXIT_HELP){
        HelpRefit();
        continue;
      }
      
      //EjectVolume
      if (MenuExit == MENU_EXIT_EJECT){
        if ((ChosenEntry->Tag == TAG_LOADER) ||
            (ChosenEntry->Tag == TAG_LEGACY)) {
          Status = EjectVolume(((LOADER_ENTRY *)ChosenEntry)->Volume);
          if (!EFI_ERROR(Status)) {
            break; //main loop is broken so Reinit all
          }
        }      
        continue;
      }
            
      // We don't allow exiting the main menu with the Escape key.
      if (MenuExit == MENU_EXIT_ESCAPE)
        break;   //refresh main menu
      //           continue;
      
      switch (ChosenEntry->Tag) {
          
        case TAG_RESET:    // Restart
          TerminateScreen();
          gRS->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
          MainLoopRunning = FALSE;   // just in case we get this far
          break;
          
        case TAG_SHUTDOWN: // Shut Down
          TerminateScreen();
 //         gRS->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
          MainLoopRunning = FALSE;   // just in case we get this far
          ReinitDesktop = FALSE;
          AfterTool = TRUE;
          break;
          
        case TAG_OPTIONS:    // Options like KernelFlags, DSDTname etc.
          OptionsMenu(&OptionEntry);
          //ApplyInputs();
          break;
          
        case TAG_ABOUT:    // About rEFIt
          AboutRefit();
          break;
          
        case TAG_LOADER:   // Boot OS via .EFI loader
          StartLoader((LOADER_ENTRY *)ChosenEntry);
          break;
          
        case TAG_LEGACY:   // Boot legacy OS
          StartLegacy((LEGACY_ENTRY *)ChosenEntry);
          break;
          
        case TAG_TOOL:     // Start a EFI tool
          StartTool((LOADER_ENTRY *)ChosenEntry);
          TerminateScreen(); //does not happen
          //   return EFI_SUCCESS;
          //  BdsLibConnectAllDriversToAllControllers();
          //    PauseForKey(L"Returned from StartTool\n");
          MainLoopRunning = FALSE;
          AfterTool = TRUE;
          break;
          
        case TAG_CLOVER:     // Clover options
          LoaderEntry = (LOADER_ENTRY *)ChosenEntry;
          if (LoaderEntry->LoadOptions != NULL) {
            if (StrStr(LoaderEntry->LoadOptions, L"BO-ADD") != NULL) {
              PrintBootOptions(FALSE);
              Status = AddBootOptionForFile (LoaderEntry->Volume->DeviceHandle,
                                             LoaderEntry->LoaderPath,
                                             TRUE,
                                             L"Clover OS X Boot",
                                             0,
                                             NULL
                                             );
              PrintBootOptions(FALSE);
            } else if (StrStr(LoaderEntry->LoadOptions, L"BO-REMOVE") != NULL) {
              PrintBootOptions(FALSE);
              Status = DeleteBootOptionForFile (LoaderEntry->Volume->DeviceHandle,
                                                LoaderEntry->LoaderPath
                                                );
              PrintBootOptions(FALSE);
            } else if (StrStr(LoaderEntry->LoadOptions, L"BO-PRINT") != NULL) {
              PrintBootOptions(TRUE);
            }
          }
          MainLoopRunning = FALSE;
          AfterTool = FALSE;
          break;
          
      } //switch
    } //MainLoopRunning
    if (!AfterTool) {
      UninitRefitLib();
      //   PauseForKey(L"After uninit");
      //reconnectAll
      if (!gFirmwareClover) {
        BdsLibConnectAllEfi();
      }
      else {
        DBG("ConnectAll after refresh menu\n");
        BdsLibConnectAllDriversToAllControllers();
      }
      //  ReinitRefitLib();
      //    PauseForKey(L"After ReinitRefitLib");      
    }
    ReinitSelfLib();
//    PauseForKey(L"After ReinitSelfLib"); 
  } while (ReinitDesktop);
  
  // If we end up here, things have gone wrong. Try to reboot, and if that
  // fails, go into an endless loop.
  //Slice - NO!!! Return to EFI GUI
  //   gRS->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
  //   EndlessIdleLoop();
  
  return EFI_SUCCESS;
}
