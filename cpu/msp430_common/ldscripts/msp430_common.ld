/*
 * Copyright (C) 2021 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

SECTIONS
{
    /* Populate information about rom size */
    _srom = ORIGIN(ROM);
    _erom = ORIGIN(ROM) + LENGTH(ROM);

    .flash_writable (NOLOAD) : {
        KEEP(*(SORT(.flash_writable.*)))
    } > ROM

    .end_fw (NOLOAD) : ALIGN(4) {
        _end_fw = . ;
    } > ROM

    /*
     * TLS relocations are offsets relative to the address
     * of the first TLS symbol. That means we just need to
     * allocate them all together so that the TLS region
     * is compact when allocated for each thread.
     */

    /*
     * TLS initialization data is loaded into ROM so that
     * each thread can get its values initialized from there
     * at startup
     */
    .tdata :
    {
         __tdata_start = .;
        *(.tdata .tdata.* .gnu.linkonce.td.*)
        __tdata_end = .;
    } > ROM
    __tdata_source = LOADADDR(.tdata);
    __tdata_size = SIZEOF(.tdata);

    /*
     * TLS zeroed data is relocated as if it immediately followed
     * the tdata values. However, the linker 'magically' erases the
     * memory allocation so that no ROM is consumed by this
     * section
     */
    .tbss :
    {
        *(.tbss .tbss.* .gnu.linkonce.tb.*)
        *(.tcommon)
        __tbss_end = .;
    } > ROM
    __tls_size = __tbss_end - __tdata_start;
    __tbss_size = __tls_size - __tdata_size;
    PROVIDE( __tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)) );

    /* compatibility with picolibc */
    PROVIDE( __data_size = __dataend - __datastart );
    PROVIDE( __data_source = LOADADDR(.data) );
    PROVIDE( __bss_size = __bsssize );
}
