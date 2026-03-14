/* ========================================================================== */
/* Master Linker Script for ARM Cortex-M Series (e.g., M4F/M7)                */
/* ========================================================================== */
/* This script defines the physical memory layout of the microcontroller and  */
/* dictates exactly where the linker must place specific code and data sections.*/
/* Crucially for safety-critical systems, it implements:                      */
/* 1. Explicit boundaries to detect stack overflows.                          */
/* 2. Uninitialized sections for post-reset crash forensics (.noinit).        */
/* 3. Un-cached memory regions for safe DMA operations (.dma_buffer).         */
/* ========================================================================== */

/* -------------------------------------------------------------------------- */
/* Memory Map Definition                                                      */
/* -------------------------------------------------------------------------- */
/* Adjust ORIGIN and LENGTH to match the exact datasheet of your specific MCU.*/
MEMORY
{
    /* Primary Flash Memory: Read-only, execute-in-place (XIP). Stores code & consts. */
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 512K
    
    /* Primary SRAM: Read/Write execution memory. */
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 128K
    
    /* Dedicated DMA RAM (for Cortex-M7 caching issues). Often mapped to distinct SRAM bank. */
    RAM_DMA (xrw)   : ORIGIN = 0x30000000, LENGTH = 32K
}

/* -------------------------------------------------------------------------- */
/* System Configuration & Stack/Heap Sizes                                    */
/* -------------------------------------------------------------------------- */
/* The Entry Point: The hardware vector table points to this symbol to boot.  */
ENTRY(Reset_Handler)

/* Exact allocations for Stack and Heap. 
   Defining these explicitly prevents silent overlap and memory corruption. */
_Min_Heap_Size  = 0x200;  /* 512 Bytes. In strict MISRA, dynamic allocation is banned. */
_Min_Stack_Size = 0x800;  /* 2048 Bytes. Sized via worst-case stack analysis (WCSA). */

/* -------------------------------------------------------------------------- */
/* Section Placement Rules                                                    */
/* -------------------------------------------------------------------------- */
SECTIONS
{
    /* 1. Interrupt Vector Table */
    /* Must be placed at the very beginning of FLASH per ARM architecture. */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector)) /* KEEP prevents the garbage collector from stripping this */
        . = ALIGN(4);
    } > FLASH

    /* 2. Program Code (.text) */
    .text :
    {
        . = ALIGN(4);
        *(.text)           /* General code */
        *(.text*)          /* Code from specific sections (-ffunction-sections) */
        
        /* Initialization routines for C++ (optional, but good practice) */
        KEEP (*(.init))
        KEEP (*(.fini))

        . = ALIGN(4);
        _etext = .;        /* Global symbol marking the end of code */
    } > FLASH

    /* 3. Read-Only Data (.rodata) */
    /* Stores const variables, string literals, and look-up tables. */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)         
        *(.rodata*)        
        . = ALIGN(4);
    } > FLASH

    /* 4. Initialized Data (.data) */
    /* Contains global/static variables initialized to non-zero values.
       These reside in RAM, but their initial values are stored in FLASH.
       The startup code copies from _sidata (FLASH) to _sdata (RAM). */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* Symbol: start of data in RAM */
        *(.data)           
        *(.data*)          
        . = ALIGN(4);
        _edata = .;        /* Symbol: end of data in RAM */
    } > RAM AT> FLASH      /* Placed in RAM, but loaded from FLASH */

    /* 5. Uninitialized Data (.bss) */
    /* Contains global/static variables initialized to zero.
       The startup code zeroes this region. Placed in RAM. */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;         /* Symbol: start of bss */
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;         /* Symbol: end of bss */
        __bss_end__ = _ebss;
    } > RAM

    /* 6. Crash Forensics / Persistence (.noinit) */
    /* Extremely important for safety-critical design. Variables placed here
       are NOT initialized by the C startup code. They survive soft resets 
       (e.g., watchdog timeouts). Used to store crash logs and fault states. */
    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.noinit*)
        . = ALIGN(4);
    } > RAM

    /* 7. Un-cached DMA Buffers (.dma_buffer) */
    /* For Cortex-M7 processors with D-Cache, standard RAM cannot be safely used
       for DMA without complex cache invalidation. We place DMA buffers in a 
       dedicated memory region configured via the MPU as un-cached. */
    .dma_buffer (NOLOAD) :
    {
        . = ALIGN(4);
        *(.dma_buffer*)
        . = ALIGN(4);
    } > RAM_DMA

    /* 8. Stack and Heap Validation */
    /* Placed at the very end of RAM. If the stack grows downwards and exceeds
       its boundary, we want to know during linking, not runtime. */
    ._user_heap_stack :
    {
        . = ALIGN(8);      /* ARM requires 8-byte stack alignment */
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /* Top of Stack Symbol */
    /* Placed at the absolute top of the primary RAM region. */
    _estack = ORIGIN(RAM) + LENGTH(RAM);
    
    /* Optional: Hardware Stack Overflow Detection (if MPU is available)
       We can place an empty "redzone" section here that the MPU configures
       as no-access. If the stack hits it, a MemManage fault is instantly triggered. */
}
