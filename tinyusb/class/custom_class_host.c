/**************************************************************************/
/*!
    @file     custom_class_host.c
    @author   hathach (tinyusb.org)

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2013, hathach (tinyusb.org)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    This file is part of the tinyusb stack.
*/
/**************************************************************************/

#include "tusb_option.h"

#if (MODE_HOST_SUPPORTED && TUSB_CFG_HOST_CUSTOM_CLASS)

#define _TINY_USB_SOURCE_FILE_

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "common/common.h"
#include "custom_class.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+
custom_interface_info_t custom_interface[TUSB_CFG_HOST_DEVICE_MAX];

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
tusb_error_t tusbh_custom_read(uint8_t dev_addr, uint16_t vendor_id, uint16_t product_id, void * p_buffer, uint16_t length)
{
  if ( !tusbh_custom_is_mounted(dev_addr, vendor_id, product_id) )
  {
    return TUSB_ERROR_DEVICE_NOT_READY;
  }

  ASSERT( p_buffer != NULL && length != 0, TUSB_ERROR_INVALID_PARA);
  if ( !hcd_pipe_is_idle(custom_interface[dev_addr-1].pipe_in) )
  {
    return TUSB_ERROR_INTERFACE_IS_BUSY;
  }

  (void) hcd_pipe_xfer( custom_interface[dev_addr-1].pipe_in, p_buffer, length, true);

  return TUSB_ERROR_NONE;
}

tusb_error_t tusbh_custom_write(uint8_t dev_addr, uint16_t vendor_id, uint16_t product_id, void const * p_data, uint16_t length)
{
  return TUSB_ERROR_NONE;
}

//--------------------------------------------------------------------+
// USBH-CLASS API
//--------------------------------------------------------------------+
void cush_init(void)
{
  memclr_(&custom_interface, sizeof(custom_interface_info_t) * TUSB_CFG_HOST_DEVICE_MAX);
}

tusb_error_t cush_open_subtask(uint8_t dev_addr, tusb_descriptor_interface_t const *p_interface_desc, uint16_t *p_length)
{
  // FIXME quick hack to test lpc1k custom class with 2 bulk endpoints
  uint8_t const *p_desc = (uint8_t const *) p_interface_desc;

  //------------- 1st Bulk Endpiont Descriptor -------------//
  for(uint32_t i=0; i<2; i++)
  {
    p_desc += p_desc[DESCRIPTOR_OFFSET_LENGTH];
    tusb_descriptor_endpoint_t const *p_endpoint = (tusb_descriptor_endpoint_t const *) p_desc;
    ASSERT_INT(TUSB_DESC_TYPE_ENDPOINT, p_endpoint->bDescriptorType, TUSB_ERROR_INVALID_PARA);

    pipe_handle_t * pipe_hdl =  ( p_endpoint->bEndpointAddress &  TUSB_DIR_DEV_TO_HOST_MASK ) ?
                         &custom_interface[dev_addr-1].pipe_in : &custom_interface[dev_addr-1].pipe_out;
    *pipe_hdl = hcd_pipe_open(dev_addr, p_endpoint, TUSB_CLASS_VENDOR_SPECIFIC);
    ASSERT ( pipehandle_is_valid(*pipe_hdl), TUSB_ERROR_HCD_OPEN_PIPE_FAILED );
  }

  (*p_length) = sizeof(tusb_descriptor_interface_t) + 2*sizeof(tusb_descriptor_endpoint_t);
  return TUSB_ERROR_NONE;
}

void cush_isr(pipe_handle_t pipe_hdl, tusb_event_t event)
{

}

void cush_close(uint8_t dev_addr)
{
  tusb_error_t err1, err2;
  custom_interface_info_t * p_interface = &custom_interface[dev_addr-1];

  // TODO re-consider to check pipe valid before calling pipe_close
  if( pipehandle_is_valid( p_interface->pipe_in ) )
  {
    err1 = hcd_pipe_close( p_interface->pipe_in );
  }

  if ( pipehandle_is_valid( p_interface->pipe_out ) )
  {
    err2 = hcd_pipe_close( p_interface->pipe_out );
  }

  memclr_(p_interface, sizeof(custom_interface_info_t));

  ASSERT(err1 == TUSB_ERROR_NONE && err2 == TUSB_ERROR_NONE, (void) 0 );
}

#endif
