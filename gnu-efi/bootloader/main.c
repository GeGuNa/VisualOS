/*
 * File:		main.c
 * *****************************************************************************
 * Copyright 2020 Scott Maday
 * You should have received a copy of the GNU General Public License along with this program. 
 * If not, see https://www.gnu.org/licenses/gpl-2.0
 */

#include <efi.h>
#include <efilib.h>
#include <elf.h>
#include "bootloader.h"

typedef unsigned long long	size_t;

// Headers for functions.c
int mem_compare(const void*, const void*, unsigned long long);
EFI_FILE* load_file(EFI_FILE*, CHAR16*, EFI_HANDLE, EFI_SYSTEM_TABLE*);

// Global variables for efi_main
EFI_HANDLE ImageHandle;
EFI_SYSTEM_TABLE* SystemTable;

// Global structs
struct FrameBuffer frame_buffer;
struct UefiKernelInterface interface;

// Uefi-kernel Interface functions
void* bootloader_load_file(void* directory, CHAR16* path){
	return load_file((EFI_FILE*)directory, path, ImageHandle, SystemTable);
}
void* bootloader_malloc(UINT64 size){
	void* location;
	SystemTable->BootServices->AllocatePool(EfiLoaderData, size, &location);
	return location;
}
void bootloader_mfree(void* location){
	SystemTable->BootServices->FreePool(location);
}

// Main code
void initalize_gop(){
	EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
	EFI_STATUS status = uefi_call_wrapper(BS->LocateProtocol, 3, &gop_guid, NULL, (void**)&gop);
	if(EFI_ERROR(status)){
		Print(L"Cannot locate GOP\n\r");
		return;
	}
	Print(L"GOP located\n\r");
	// Frame buffer
	frame_buffer.base_ptr = (void*)gop->Mode->FrameBufferBase;
	frame_buffer.size  = gop->Mode->FrameBufferSize;
	frame_buffer.width  = gop->Mode->Info->HorizontalResolution;
	frame_buffer.height  = gop->Mode->Info->VerticalResolution;
	frame_buffer.ppsl  = gop->Mode->Info->PixelsPerScanLine;
}

EFI_STATUS efi_main (EFI_HANDLE _ImageHandle, EFI_SYSTEM_TABLE* _SystemTable) {
	ImageHandle = _ImageHandle;
	SystemTable = _SystemTable;
	// Begin loading the kernel
	InitializeLib(ImageHandle, SystemTable);	// UEFI environment to make out lives easier
	Print(L"Loading VisualOS\n\r");

	EFI_FILE* kernel = load_file(NULL, L"kernel.elf", ImageHandle, SystemTable);
	if (kernel == NULL){
		Print(L"Could not load kernel\n\r");
		return EFI_LOAD_ERROR;
	}
	Print(L"Loaded kernel\n\r");

	// Load binary header and check
	Elf64_Ehdr header;
	{
		UINTN file_info_size;
		EFI_FILE_INFO* file_info;
		kernel->GetInfo(kernel, &gEfiFileInfoGuid, &file_info_size, NULL);
		SystemTable->BootServices->AllocatePool(EfiLoaderData, file_info_size, (void**)&file_info);
		kernel->GetInfo(kernel, &gEfiFileInfoGuid, &file_info_size, (void**)&file_info);
		UINTN size = sizeof(header);
		kernel->Read(kernel, &size, &header);
	}
	if( mem_compare(&header.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
		header.e_ident[EI_CLASS] != ELFCLASS64 ||
		header.e_ident[EI_DATA] != ELFDATA2LSB ||
		header.e_type != ET_EXEC ||
		header.e_machine != EM_X86_64 ||
		header.e_version != EV_CURRENT
	){
		Print(L"Kernel format is bad\n\r");
		return EFI_UNSUPPORTED;
	}
	Print(L"Kernel verified\n\r");

	// Load binary program headers
	Elf64_Phdr* pheaders;
	{
		kernel->SetPosition(kernel, header.e_phoff);
		UINTN size = header.e_phnum * header.e_phentsize;
		SystemTable->BootServices->AllocatePool(EfiLoaderData, size, (void**)&pheaders);
		kernel->Read(kernel, &size, pheaders);
	}
	for( Elf64_Phdr* ph_ptr = pheaders;
		(char*)ph_ptr < (char*)pheaders + header.e_phnum * header.e_phentsize;
		ph_ptr = (Elf64_Phdr*)((char*)ph_ptr + header.e_phentsize)
	){
		switch(ph_ptr->p_type) {
			case PT_LOAD: {
				int pages = (ph_ptr->p_memsz + 0x1000 - 1) / 0x1000;	// get size to nearest 4K and round up
				Elf64_Addr seg = ph_ptr->p_paddr;
				SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, pages, &seg);
				kernel->SetPosition(kernel, ph_ptr->p_offset);
				UINTN size = ph_ptr->p_filesz;
				kernel->Read(kernel, &size, (void*)seg);
				Print(L"Kernel program header read (PT_LOAD)\n\r");
				break;
			}
			default: {
				Print(L"Kernel program header read (Unknown=%u)\n\r", ph_ptr->p_type);
			}
		}
	}
	// Get important values for the kernel
	initalize_gop();
	Print(L"GOP frame buffer data:\n\r"
		"  Base=0x%x\n\r"
		"  Size=%x\n\r"
		"  Width=%d\n\r"
		"  Height=%d\n\r"
		"  Pixels per scanline=%d\n\r", 
	frame_buffer.base_ptr, frame_buffer.size, frame_buffer.width, frame_buffer.height, frame_buffer.ppsl);

	// Populate the kernel interface
	interface = (struct UefiKernelInterface){
		.frame_buffer_ptr = &frame_buffer,
		.malloc_ptr = &bootloader_malloc,
		.mfree_ptr = &bootloader_mfree,
		.load_file_ptr = &bootloader_load_file
	};

	// Enter into the kernel
	void (*kernel_start)(struct UefiKernelInterface*) = ((__attribute__((sysv_abi)) void(*)()) header.e_entry);
	kernel_start(&interface);
	//load_file(NULL, L"kernel.elf", ImageHandle, SystemTable);

	return EFI_SUCCESS; // Exit the UEFI application
}
