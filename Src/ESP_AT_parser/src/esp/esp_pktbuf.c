#include "esp/esp.h"
#include "esp/esp_private.h"

extern espGlbl_t espGlobal;



espPbuf_t*  pxESPpktBufCreate( size_t len )
{
    espPbuf_t *buf_p   = NULL;
    uint8_t   *payload = NULL; 

    buf_p = (uint8_t *) ESP_MALLOC( sizeof(espPbuf_t) + len );
    ESP_ASSERT( buf_p != NULL );
    payload = ((uint8_t *)buf_p) + sizeof(espPbuf_t); 
    buf_p->next = NULL; 
    buf_p->payload_len = len;   
    buf_p->chain_len   = 0; 
    buf_p->payload     = payload; 
    return  buf_p;
} // end of pxESPpktBufCreate




espRes_t     eESPpktBufCopy( espPbuf_t *des_p, void *src_p , size_t len )
{
    espRes_t  response    =  espOK ;
    size_t    cpy_len = ESP_MIN( len, des_p->payload_len );
    ESP_MEMCPY( (void *)des_p->payload, src_p, cpy_len );
    if(cpy_len != len) { 
        // if CPU gets here, that means len is greater than payload size,
        // some of received IPD data are missing because of insufficient payload space.
        response = espERR;
    }
    return response;
} // end of eESPpktBufCopy




void     vESPpktBufItemDelete ( espPbuf_t  *buff )
{
    ESP_ASSERT( buff != NULL );
    ESP_MEMFREE( buff );
} // end of vESPpktBufItemDelete




void    vESPpktBufChainDelete( espPbuf_t *buff_head )
{
    espPbuf_t  *curr_p   =  buff_head;
    espPbuf_t  *next_p   =  NULL;

    while( curr_p != NULL ) {
        next_p = curr_p->next;
        vESPpktBufItemDelete( curr_p );
        curr_p = next_p ;
    }
} // end of vESPpktBufChainDelete





