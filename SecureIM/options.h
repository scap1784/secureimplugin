#ifndef __OPTIONS_H__
#define __OPTIONS_H__

BOOL CALLBACK OptionsDlgProc(HWND,UINT,WPARAM,LPARAM);
BOOL CALLBACK DlgProcOptionsGeneral(HWND,UINT,WPARAM,LPARAM);
BOOL CALLBACK DlgProcOptionsProto(HWND,UINT,WPARAM,LPARAM);
BOOL CALLBACK DlgProcOptionsPGP(HWND,UINT,WPARAM,LPARAM);
BOOL CALLBACK DlgProcOptionsGPG(HWND,UINT,WPARAM,LPARAM);
BOOL CALLBACK DlgProcSetPSK(HWND,UINT,WPARAM,LPARAM);
void ApplyGeneralSettings(HWND);
void ApplyProtoSettings(HWND);
void ApplyPGPSettings(HWND);
void ApplyGPGSettings(HWND);
void RefreshGeneralDlg(HWND,UINT);
void RefreshProtoDlg(HWND);
void RefreshPGPDlg(HWND,BOOL);
void RefreshGPGDlg(HWND,BOOL);
void ResetGeneralDlg(HWND);
void ResetProtoDlg(HWND);
LPARAM getListViewParam(HWND,UINT);
void setListViewIcon(HWND,UINT,pUinKey);
void setListViewMode(HWND,UINT,UINT);
void setListViewStatus(HWND,UINT,UINT);
UINT getListViewPSK(HWND,UINT);
void setListViewPSK(HWND,UINT,UINT);
int onRegisterOptions(WPARAM,LPARAM);
int CALLBACK CompareFunc(LPARAM,LPARAM,LPARAM);
void ListView_Sort(HWND,LPARAM);
BOOL ShowSelectKeyDlg(HWND,LPSTR);
LPSTR LoadKeys(LPCSTR,BOOL);

#define getListViewContact(h,i)	(HANDLE)getListViewParam(h,i)
#define getListViewProto(h,i)	(char*)getListViewParam(h,i)

#endif
