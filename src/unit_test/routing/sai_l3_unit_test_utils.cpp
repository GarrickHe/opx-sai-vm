/************************************************************************
* LEGALESE:   "Copyright (c) 2015, Dell Inc. All rights reserved."
*
* This source code is confidential, proprietary, and contains trade
* secrets that are the sole property of Dell Inc.
* Copy and/or distribution of this source code or disassembly or reverse
* engineering of the resultant object code are strictly forbidden without
* the written consent of Dell Inc.
*
************************************************************************/
/**
* @file  sai_l3_unit_test_utils.cpp
*
* @brief This file contains utility and helper function definitions for
*        testing the SAI L3 functionalities.
*
*************************************************************************/

#include "gtest/gtest.h"

#include "sai_l3_unit_test_utils.h"
#include "sai_bridge_unit_test_utils.h"

extern "C" {
#include "sai.h"
#include "saitypes.h"
#include "saistatus.h"
#include "saiswitch.h"
#include "saivirtualrouter.h"
#include "sairouterinterface.h"
#include "saineighbor.h"
#include "sainexthop.h"
#include "sainexthopgroup.h"
#include "sairoute.h"
#include <arpa/inet.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
}

#define SAI_TEST_MAX_GROUP_NEXT_HOP_MEMBER_ATTR (SAI_NEXT_HOP_GROUP_MEMBER_ATTR_END - SAI_NEXT_HOP_GROUP_MEMBER_ATTR_START)
#define SAI_MAX_MAC_LENGTH 6

/* Definition for the data members */
sai_switch_api_t* saiL3Test ::p_sai_switch_api_tbl = NULL;
sai_vlan_api_t* saiL3Test ::p_sai_vlan_api_tbl = NULL;
sai_virtual_router_api_t* saiL3Test ::p_sai_vrf_api_tbl = NULL;
sai_router_interface_api_t* saiL3Test ::p_sai_rif_api_tbl = NULL;
sai_neighbor_api_t* saiL3Test ::p_sai_nbr_api_tbl = NULL;
sai_next_hop_api_t* saiL3Test ::p_sai_nh_api_tbl = NULL;
sai_next_hop_group_api_t* saiL3Test ::p_sai_nh_grp_api_tbl = NULL;
sai_route_api_t* saiL3Test ::p_sai_route_api_tbl = NULL;
sai_fdb_api_t* saiL3Test ::p_sai_fdb_api_table = NULL;
sai_bridge_api_t* saiL3Test ::p_sai_bridge_api_tbl = NULL;
sai_l3_test_nh_grp_info_2_member_t saiL3Test::_nh_grp_info_2_member;
sai_l3_test_member_2_nh_grp_info_t saiL3Test::_nh_member_2_grp_info;
const char* saiL3Test ::router_mac = "00:a0:a1:a2:a3:a4";
unsigned int saiL3Test::port_count = 0;
sai_object_id_t saiL3Test::port_list[SAI_TEST_MAX_PORTS] = {0};
sai_object_id_t saiL3Test::default_vlan_id = 0;
sai_object_id_t saiL3Test::switch_id = 0;
unsigned int saiL3Test::bridge_port_count = 0;
sai_object_id_t saiL3Test::bridge_port_list[SAI_TEST_MAX_PORTS] = {0};
sai_object_id_t saiL3Test::default_bridge_id = 0;

/*
 * Stubs for Callback functions to be passed from adapter host/application.
 */
static inline void sai_port_state_evt_callback (uint32_t count,
                                                sai_port_oper_status_notification_t *data)
{
}

static inline void sai_fdb_evt_callback (uint32_t count,
                                         sai_fdb_event_notification_data_t *data)
{
}

static inline void sai_switch_operstate_callback (sai_switch_oper_status_t
                                                  switchstate)
{
}

static inline void sai_packet_event_callback (const void *buffer,
                                              sai_size_t buffer_size,
                                              uint32_t attr_count,
                                              const sai_attribute_t *attr_list)
{
}

static inline void sai_switch_shutdown_callback (void)
{
}

sai_status_t saiL3Test ::sai_test_l3_create_bridge_port(sai_object_id_t port_id,
                                           sai_object_id_t *bridge_port_id)
{

    return sai_bridge_ut_create_bridge_port(p_sai_bridge_api_tbl, switch_id,
                                            port_id, false,  bridge_port_id);
}

sai_status_t saiL3Test ::sai_test_l3_remove_bridge_port(sai_object_id_t bridge_port_id)
{
    return sai_bridge_ut_remove_bridge_port(p_sai_bridge_api_tbl, switch_id,
                                            bridge_port_id, false);
}

sai_object_id_t saiL3Test ::sai_test_l3_add_lag_member (
                    sai_lag_api_t *sai_lag_api_table,
                    sai_object_id_t lag_id, sai_object_id_t port_id)
{
    unsigned int count = 0;
    sai_object_id_t member_id;
    sai_attribute_t attr_list [2];
    sai_object_id_t bridge_port_id = 0;

    EXPECT_EQ(SAI_STATUS_SUCCESS,
              sai_bridge_ut_get_bridge_port_from_port(p_sai_switch_api_tbl, p_sai_bridge_api_tbl,
                                                  switch_id, port_id, &bridge_port_id));


    attr_list [count].id = SAI_LAG_MEMBER_ATTR_LAG_ID;
    attr_list [count].value.oid = lag_id;
    count++;

    attr_list [count].id = SAI_LAG_MEMBER_ATTR_PORT_ID;
    attr_list [count].value.oid = port_id;
    count++;

    EXPECT_EQ(SAI_STATUS_SUCCESS,
              sai_test_l3_remove_bridge_port(bridge_port_id));
    EXPECT_EQ(SAI_STATUS_SUCCESS, sai_lag_api_table->create_lag_member (&member_id, switch_id, count,
                                                                        attr_list));

    return member_id;
}

sai_status_t saiL3Test ::sai_test_l3_remove_lag_member (
                    sai_lag_api_t *sai_lag_api_table,
                    sai_object_id_t lag_member_id)
{
    sai_attribute_t attr_list [1];
    sai_status_t    sai_rc = SAI_STATUS_SUCCESS;
    sai_object_id_t bridge_port_id = 0;
    sai_object_id_t port_id = 0;

    attr_list [0].id = SAI_LAG_MEMBER_ATTR_PORT_ID;

    sai_rc = sai_lag_api_table->get_lag_member_attribute (lag_member_id, 1,
                                                          attr_list);
    if(sai_rc != SAI_STATUS_SUCCESS) {
        return sai_rc;
    }
    port_id = attr_list [0].value.oid;
    sai_rc = sai_lag_api_table->remove_lag_member(lag_member_id);

    if(sai_rc != SAI_STATUS_SUCCESS) {
        return sai_rc;
    }

    return sai_test_l3_create_bridge_port(port_id, &bridge_port_id);
}
/* SAI switch initialization */
void saiL3Test ::SetUpTestCase (void)
{
    sai_attribute_t attr;
    sai_status_t sai_rc = SAI_STATUS_SUCCESS;


    /*
     * Query and populate the SAI Switch API Table.
     */
    EXPECT_EQ (SAI_STATUS_SUCCESS, sai_api_query
               (SAI_API_SWITCH, (static_cast<void**>
                                 (static_cast<void*>(&p_sai_switch_api_tbl)))));

    ASSERT_TRUE (p_sai_switch_api_tbl != NULL);


    sai_attribute_t sai_attr_set[7];
    uint32_t attr_count = 7;

    memset(&sai_attr_set,0, sizeof(sai_attribute_t));

    sai_attr_set[0].id = SAI_SWITCH_ATTR_INIT_SWITCH;
    sai_attr_set[0].value.booldata = 1;

    sai_attr_set[1].id = SAI_SWITCH_ATTR_SWITCH_PROFILE_ID;
    sai_attr_set[1].value.u32 = 0;

    sai_attr_set[2].id = SAI_SWITCH_ATTR_FDB_EVENT_NOTIFY;
    sai_attr_set[2].value.ptr = (void *)sai_fdb_evt_callback;

    sai_attr_set[3].id = SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY;
    sai_attr_set[3].value.ptr = (void *)sai_port_state_evt_callback;

    sai_attr_set[4].id = SAI_SWITCH_ATTR_PACKET_EVENT_NOTIFY;
    sai_attr_set[4].value.ptr = (void *)sai_packet_event_callback;

    sai_attr_set[5].id = SAI_SWITCH_ATTR_SWITCH_STATE_CHANGE_NOTIFY;
    sai_attr_set[5].value.ptr = (void *)sai_switch_operstate_callback;

    sai_attr_set[6].id = SAI_SWITCH_ATTR_SHUTDOWN_REQUEST_NOTIFY;
    sai_attr_set[6].value.ptr = (void *)sai_switch_shutdown_callback;

    ASSERT_TRUE(p_sai_switch_api_tbl->create_switch != NULL);

    EXPECT_EQ (SAI_STATUS_SUCCESS,
               (p_sai_switch_api_tbl->create_switch (&switch_id , attr_count,
                                                         sai_attr_set)));

    /* Query the L3 API method tables */
    SetUpL3ApiQuery ();

    /* Get the switch port count and port list */
    memset (&attr, 0, sizeof (attr));

    attr.id = SAI_SWITCH_ATTR_PORT_LIST;
    attr.value.objlist.count = SAI_TEST_MAX_PORTS;
    attr.value.objlist.list  = port_list;

    sai_rc = p_sai_switch_api_tbl->get_switch_attribute (switch_id,1, &attr);
    ASSERT_EQ (SAI_STATUS_SUCCESS, sai_rc);
    port_count = attr.value.objlist.count;

    attr.id = SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID;

    sai_rc = p_sai_switch_api_tbl->get_switch_attribute(switch_id,1,&attr);

    ASSERT_EQ (SAI_STATUS_SUCCESS, sai_rc);
    default_bridge_id = attr.value.oid;

    attr.id = SAI_BRIDGE_ATTR_PORT_LIST;
    attr.value.objlist.count = SAI_TEST_MAX_PORTS;
    attr.value.objlist.list = bridge_port_list;

    sai_rc = p_sai_bridge_api_tbl->get_bridge_attribute(default_bridge_id, 1, &attr);

    ASSERT_EQ (SAI_STATUS_SUCCESS, sai_rc);
    bridge_port_count = attr.value.objlist.count;

    printf ("Switch port count is %u.\r\n", port_count);
    printf ("Bridge port count is %u.\r\n", bridge_port_count);

    /* Get the default VLAN OBJ ID */
    memset (&attr, 0, sizeof (attr));

    attr.id = SAI_SWITCH_ATTR_DEFAULT_VLAN_ID;
    sai_rc = p_sai_switch_api_tbl->get_switch_attribute (switch_id,1, &attr);
    ASSERT_EQ (SAI_STATUS_SUCCESS, sai_rc);
    default_vlan_id = attr.value.oid;

    printf ("Default VLAN obj ID is %lu.\r\n", default_vlan_id);

    ASSERT_TRUE (port_count != 0);
    ASSERT_TRUE (default_vlan_id != 0);
}

/* SAI L3 API Query */
void saiL3Test ::SetUpL3ApiQuery (void)
{
    /*
     * Query and populate the SAI VLAN API Table.
     */
    EXPECT_EQ (SAI_STATUS_SUCCESS, sai_api_query
               (SAI_API_VLAN, (static_cast<void**>
                               (static_cast<void*>(&p_sai_vlan_api_tbl)))));

    ASSERT_TRUE (p_sai_vlan_api_tbl != NULL);

    /*
     * Query and populate the SAI Virtual Router API Table.
     */
    EXPECT_EQ (SAI_STATUS_SUCCESS, sai_api_query
               (SAI_API_VIRTUAL_ROUTER, (static_cast<void**>
                                         (static_cast<void*>
                                          (&p_sai_vrf_api_tbl)))));

    ASSERT_TRUE (p_sai_vrf_api_tbl != NULL);

    /*
     * Query and populate the SAI Router Interface API Table.
     */
    EXPECT_EQ (SAI_STATUS_SUCCESS, sai_api_query
               (SAI_API_ROUTER_INTERFACE, (static_cast<void**>
                                           (static_cast<void*>
                                            (&p_sai_rif_api_tbl)))));

    ASSERT_TRUE (p_sai_rif_api_tbl != NULL);

    /*
     * Query and populate the SAI Neighbor API Table.
     */
    EXPECT_EQ (SAI_STATUS_SUCCESS, sai_api_query
               (SAI_API_NEIGHBOR, (static_cast<void**>
                                   (static_cast<void*>
                                    (&p_sai_nbr_api_tbl)))));

    ASSERT_TRUE (p_sai_nbr_api_tbl != NULL);

    /*
     * Query and populate the SAI Next-hop API Table.
     */
    EXPECT_EQ (SAI_STATUS_SUCCESS, sai_api_query
               (SAI_API_NEXT_HOP, (static_cast<void**>
                                   (static_cast<void*>
                                    (&p_sai_nh_api_tbl)))));

    ASSERT_TRUE (p_sai_nh_api_tbl != NULL);

    /*
     * Query and populate the SAI Next-hop group API Table.
     */
    EXPECT_EQ (SAI_STATUS_SUCCESS, sai_api_query
               (SAI_API_NEXT_HOP_GROUP, (static_cast<void**>
                                         (static_cast<void*>
                                          (&p_sai_nh_grp_api_tbl)))));

    ASSERT_TRUE (p_sai_nh_grp_api_tbl != NULL);

    /*
     * Query and populate the SAI Route API Table.
     */
    EXPECT_EQ (SAI_STATUS_SUCCESS, sai_api_query
               (SAI_API_ROUTE, (static_cast<void**>
                                (static_cast<void*>
                                 (&p_sai_route_api_tbl)))));

    ASSERT_TRUE (p_sai_route_api_tbl != NULL);

    /* Query and populate the SAI FDB API table */
    EXPECT_EQ (SAI_STATUS_SUCCESS, sai_api_query
               (SAI_API_FDB, (static_cast<void**>
                              (static_cast<void*> (&p_sai_fdb_api_table)))));

    ASSERT_TRUE (p_sai_fdb_api_table != NULL);

    /*
     * Query and populate the SAI Bridge API Table.
     */
    EXPECT_EQ (SAI_STATUS_SUCCESS, sai_api_query
               (SAI_API_BRIDGE, (static_cast<void**>
                                 (static_cast<void*>(&p_sai_bridge_api_tbl)))));

    ASSERT_TRUE (p_sai_bridge_api_tbl != NULL);

}

sai_object_id_t saiL3Test ::sai_l3_port_id_get (uint32_t port_index)
{
    if(port_index >= port_count) {
        return 0;
    }

    return port_list [port_index];
}

sai_object_id_t saiL3Test ::sai_l3_bridge_port_id_get (uint32_t bridge_port_index)
{
    if(bridge_port_index >= bridge_port_count) {
        return 0;
    }

    return bridge_port_list [bridge_port_index];
}


sai_object_id_t saiL3Test ::sai_l3_invalid_port_id_get ()
{
    return (port_list[port_count-1] + 1);
}

void saiL3Test ::sai_test_router_mac_str_to_bytes_get (const char *mac_str,
                                                       uint8_t *mac)
{
    unsigned int mac_in [6];
    unsigned int byte_idx = 0;

    sscanf (mac_str, "%x:%x:%x:%x:%x:%x", &mac_in [0], &mac_in [1],
            &mac_in [2], &mac_in [3], &mac_in [4], &mac_in [5]);

    printf ("MAC: ");

    for (byte_idx = 0; byte_idx < 6; byte_idx++) {
        mac [byte_idx] = mac_in [byte_idx];

        printf ("%x", mac_in [byte_idx]);

        if (byte_idx != 5) {
            printf (":");
        }
    }

    printf ("\r\n");
}

sai_status_t saiL3Test ::sai_test_router_mac_set (const char *mac_str)
{
    sai_status_t    sai_rc = SAI_STATUS_SUCCESS;
    uint8_t         sai_mac [6];
    sai_attribute_t attr;

    sai_test_router_mac_str_to_bytes_get (mac_str, sai_mac);

    attr.id = SAI_SWITCH_ATTR_SRC_MAC_ADDRESS;

    memcpy (attr.value.mac, sai_mac, sizeof (sai_mac_t));

    sai_rc = p_sai_switch_api_tbl->set_switch_attribute
        (switch_id,(const sai_attribute_t *)&attr);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("Switch MAC set failed.\r\n");
    } else {
        printf ("Switch MAC set success.\r\n");
    }

    return sai_rc;
}

sai_status_t saiL3Test ::sai_test_router_mac_get (sai_mac_t *p_switch_mac)
{
    sai_status_t    sai_rc = SAI_STATUS_SUCCESS;
    sai_attribute_t attr;

    attr.id = SAI_SWITCH_ATTR_SRC_MAC_ADDRESS;

    sai_rc = p_sai_switch_api_tbl->get_switch_attribute (switch_id,1, &attr);

    memcpy (p_switch_mac, attr.value.mac, sizeof (sai_mac_t));

    return sai_rc;
}

sai_status_t saiL3Test ::sai_test_router_mac_init (const char *mac_str)
{
    sai_status_t sai_rc = SAI_STATUS_SUCCESS;
    sai_mac_t    zero_mac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    sai_mac_t    sai_mac;

    sai_test_router_mac_get (&sai_mac);

    /* Set Router MAC, if it is not initialized */
    if ((memcmp (sai_mac, zero_mac, sizeof (sai_mac_t))) == 0) {
        sai_rc = sai_test_router_mac_set (mac_str);
    }

    return sai_rc;
}

sai_status_t saiL3Test ::sai_test_vlan_create (sai_object_id_t *vlan_obj_id,
        unsigned int vlan_id)
{
    sai_status_t sai_rc = SAI_STATUS_SUCCESS;
    sai_attribute_t attr;

    attr.id = SAI_VLAN_ATTR_VLAN_ID;
    attr.value.u16 = vlan_id;
    sai_rc = p_sai_vlan_api_tbl->create_vlan(vlan_obj_id,0,1,&attr);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI VLAN Creation API failed with error: %d\r\n", sai_rc);
    } else {
        printf ("SAI VLAN Creation API success, VLAN ID: %d\r\n", vlan_id);
    }

    return sai_rc;
}

sai_status_t saiL3Test ::sai_test_vlan_remove (sai_object_id_t vlan_obj_id)
{
    sai_status_t sai_rc = SAI_STATUS_SUCCESS;

    sai_rc = p_sai_vlan_api_tbl->remove_vlan (vlan_obj_id);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI VLAN Remove API failed with error: %d\r\n", sai_rc);
    } else {
        printf ("SAI VLAN Remove API success, VLAN OBJ ID: %ld\r\n", vlan_obj_id);
    }

    return sai_rc;
}

static const char* sai_test_vrf_attr_id_to_name_get (unsigned int attr_id) {
    if (SAI_VIRTUAL_ROUTER_ATTR_ADMIN_V4_STATE == attr_id) {
        return "VRF V4 Admin State";
    } else if (SAI_VIRTUAL_ROUTER_ATTR_ADMIN_V6_STATE == attr_id) {
        return "VRF V6 Admin State";
    } else if (SAI_VIRTUAL_ROUTER_ATTR_SRC_MAC_ADDRESS == attr_id) {
        return "VRF SRC MAC";
    } else if (SAI_VIRTUAL_ROUTER_ATTR_VIOLATION_TTL1_PACKET_ACTION == attr_id) {
        return "VRF TTL1 PKT ACTION";
    } else if (SAI_VIRTUAL_ROUTER_ATTR_VIOLATION_IP_OPTIONS_PACKET_ACTION == attr_id) {
        return "VRF IP OPTIONS PKT ACTION";
    } else {
        return "INVALID/UNKNOWN";
    }
}

static void sai_test_vrf_attr_value_fill (sai_attribute_t *p_attr,
                                          unsigned int attr_val,
                                          const char *mac_str)
{
    uint8_t sai_mac [6];

        switch (p_attr->id) {
            case SAI_VIRTUAL_ROUTER_ATTR_ADMIN_V4_STATE:
            case SAI_VIRTUAL_ROUTER_ATTR_ADMIN_V6_STATE:
                p_attr->value.booldata = attr_val;

                printf ("Value: %d\r\n", p_attr->value.booldata);
                break;

            case SAI_VIRTUAL_ROUTER_ATTR_SRC_MAC_ADDRESS:
                saiL3Test ::sai_test_router_mac_str_to_bytes_get (mac_str,
                                                                  sai_mac);

                memcpy (p_attr->value.mac, sai_mac, sizeof (sai_mac_t));

                break;

            case SAI_VIRTUAL_ROUTER_ATTR_VIOLATION_TTL1_PACKET_ACTION:
            case SAI_VIRTUAL_ROUTER_ATTR_VIOLATION_IP_OPTIONS_PACKET_ACTION:
                p_attr->value.s32 = attr_val;

                printf ("Value: %d\r\n", p_attr->value.s32);
                break;

            default:
                p_attr->value.u64 = attr_val;

                printf ("Value: %d\r\n", (int)p_attr->value.u64);
                break;
        }
}

static void sai_test_vrf_attr_value_print (sai_attribute_t *p_attr)
{
    unsigned int idx;

    switch (p_attr->id) {
        case SAI_VIRTUAL_ROUTER_ATTR_ADMIN_V4_STATE:
            printf ("VRF V4 admin State: %d\r\n", p_attr->value.booldata);
            break;

        case SAI_VIRTUAL_ROUTER_ATTR_ADMIN_V6_STATE:
            printf ("VRF V6 admin State: %d\r\n", p_attr->value.booldata);
            break;

        case SAI_VIRTUAL_ROUTER_ATTR_SRC_MAC_ADDRESS:
            printf ("VRF SAI MAC: ");

            for (idx = 0; idx < 6; idx++) {
                if (idx != 5) {
                    printf ("%x:", p_attr->value.mac [idx]);
                } else {
                    printf ("%x\r\n", p_attr->value.mac [idx]);
                }
            }

            break;

        case SAI_VIRTUAL_ROUTER_ATTR_VIOLATION_TTL1_PACKET_ACTION:
            printf ("VRF TTL1 VIOLATION PKT ACTION: %d\r\n", p_attr->value.s32);
            break;

        case SAI_VIRTUAL_ROUTER_ATTR_VIOLATION_IP_OPTIONS_PACKET_ACTION:
            printf ("VRF IP OPTIONS VIOLATION PKT ACTION: %d\r\n",
                    p_attr->value.s32);
            break;

        default:
            printf ("%d is an Invalid VRF attribute ID.\r\n", p_attr->id);
            break;
    }
}

/*
 * p_vr_id     - [out] pointer to VRF ID generated by the SAI API.
 * attr_count  - [in]  number of VRF attributes passed.
 *               For each attribute, {id, value} is passed.
 *
 * For attr-count = 2,
 * sai_test_vrf_create (p_vr_id, 2, id_0, val_0, id_1, val_1)
 */
sai_status_t saiL3Test ::sai_test_vrf_create (sai_object_id_t *p_vr_id,
                                              unsigned int attr_count, ...)
{
    sai_status_t     sai_rc = SAI_STATUS_SUCCESS;
    va_list          ap;
    unsigned int     ap_idx = 0;
    sai_attribute_t *p_attr_list = NULL;
    unsigned int     attr_id = 0;
    unsigned long    val = 0;
    char            default_mac[SAI_MAX_MAC_LENGTH] = {0};
    const char      *mac_str = NULL;

    p_attr_list
        = (sai_attribute_t *) calloc (attr_count, sizeof (sai_attribute_t));

    printf ("Testing VRF Create API with attribute count: %d\r\n", attr_count);

    if (!p_attr_list) {
        printf ("Failed to allocate memory for attribute list.\r\n");

        return SAI_STATUS_FAILURE;
    }

    va_start (ap, attr_count);

    for (ap_idx = 0; ap_idx < attr_count; ap_idx++) {
        attr_id = va_arg (ap, unsigned int);
        p_attr_list [ap_idx].id = attr_id;

        printf ("Setting List index: %d with Attribute %s (ID = %d)\r\n",
                ap_idx, sai_test_vrf_attr_id_to_name_get (attr_id), attr_id);

        if (SAI_VIRTUAL_ROUTER_ATTR_SRC_MAC_ADDRESS == attr_id) {
            mac_str = va_arg (ap, char*);
        } else {
            val = va_arg (ap, unsigned long);
            mac_str = default_mac; /* To Avoid de-reference */
        }

        sai_test_vrf_attr_value_fill (&p_attr_list [ap_idx], val, mac_str);
    }

    sai_rc = p_sai_vrf_api_tbl->create_virtual_router (p_vr_id, switch_id, attr_count,
                                                       p_attr_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI VRF Creation API failed with error: %d\r\n", sai_rc);
    } else {
        printf ("SAI VRF Creation API success, VRF ID: 0x%" PRIx64 "\r\n",
                *p_vr_id);
    }

    if (p_attr_list) {
        free (p_attr_list);
    }

    va_end (ap);
    return sai_rc;
}

sai_status_t saiL3Test ::sai_test_vrf_remove (sai_object_id_t vr_id)
{
    sai_status_t sai_rc = SAI_STATUS_SUCCESS;

    sai_rc = p_sai_vrf_api_tbl->remove_virtual_router (vr_id);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI VRF removal failed with error: %d\r\n", sai_rc);
    } else {
        printf ("SAI VRF removal success for VRF ID: 0x%" PRIx64 "\r\n", vr_id);
    }

    return sai_rc;
}

sai_status_t saiL3Test ::sai_test_vrf_attr_set (sai_object_id_t vr_id,
                                                unsigned int attr_id,
                                                unsigned long val,
                                                const char *mac_str)
{
    sai_status_t    sai_rc = SAI_STATUS_SUCCESS;
    sai_attribute_t attr;

    attr.id = attr_id;

    printf ("Setting Attribute %s (ID = %d) for VRF 0x%" PRIx64 ".\r\n",
            sai_test_vrf_attr_id_to_name_get (attr_id), attr_id, vr_id);

    sai_test_vrf_attr_value_fill (&attr, val, mac_str);

    sai_rc = p_sai_vrf_api_tbl->set_virtual_router_attribute (vr_id, &attr);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI VRF attribute set API failed with error: %d\r\n", sai_rc);
    } else {
        printf ("SAI VRF attribute set API success.\r\n");
    }

    return sai_rc;
}

/*
 * vrf_id      - [in] VRF ID.
 * p_attr_list - [in, out] pointer to attribute list.
 * attr_count  - [in] number of attributes to get.
 *                    For each attribute, SAI attribute id is passed.
 *
 * For attr_count = 2,
 * sai_test_vrf_attr_get (vrf_id, 2, p_attr_list, id_0, id_1)
 */
sai_status_t saiL3Test ::sai_test_vrf_attr_get (sai_object_id_t vrf_id,
                                                sai_attribute_t *p_attr_list,
                                                unsigned int attr_count,
                                                ...)
{
    sai_status_t sai_rc = SAI_STATUS_SUCCESS;
    va_list      ap;
    unsigned int ap_idx = 0;
    unsigned int attr_id = 0;

    printf ("Testing VRF GET API for VRF ID: 0x%" PRIx64 " with attribute count: "
            "%d\r\n", vrf_id, attr_count);

    va_start (ap, attr_count);

    /* Fill in the VRF attribute IDs for get */
    for (ap_idx = 0; ap_idx < attr_count; ap_idx++) {
        attr_id = va_arg (ap, unsigned int);
        p_attr_list [ap_idx].id = attr_id;

        printf ("Setting List index: %d with Attribute %s (ID = %d)\r\n",
                ap_idx, sai_test_vrf_attr_id_to_name_get (attr_id), attr_id);
    }

    sai_rc =
        p_sai_vrf_api_tbl->get_virtual_router_attribute (vrf_id, attr_count,
                                                         p_attr_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("VRF Get API failed with error: %d\r\n", sai_rc);
    } else {
        printf ("VRF Get API success, VRF ID: 0x%" PRIx64 "\r\n", vrf_id);

        printf ("\r\n##### VRF Attributes Details #####\r\n");

        for (ap_idx = 0; ap_idx < attr_count; ap_idx++) {
            printf ("VRF attribute list idx: %d, SAI Attribute ID: %d.\r\n",
                    ap_idx, p_attr_list [ap_idx].id);

            sai_test_vrf_attr_value_print (&p_attr_list [ap_idx]);
        }
    }

    va_end (ap);
    return sai_rc;
}

static const char* sai_test_rif_attr_id_to_name_get (unsigned int attr_id)
{
    static const std::unordered_map <int, const char*, std::hash<int>>
        _rif_attr_id_2_str_map = {
            {SAI_ROUTER_INTERFACE_ATTR_VIRTUAL_ROUTER_ID, "VRF ID"},
            {SAI_ROUTER_INTERFACE_ATTR_TYPE, "RIF TYPE"},
            {SAI_ROUTER_INTERFACE_ATTR_PORT_ID, "RIF PORT ID"},
            {SAI_ROUTER_INTERFACE_ATTR_VLAN_ID, "RIF VLAN ID"},
            {SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS, "RIF SRC MAC"},
            {SAI_ROUTER_INTERFACE_ATTR_ADMIN_V4_STATE, "RIF V4 Admin State"},
            {SAI_ROUTER_INTERFACE_ATTR_ADMIN_V6_STATE, "RIF V6 Admin State"},
            {SAI_ROUTER_INTERFACE_ATTR_MTU, "RIF MTU"},
            {SAI_ROUTER_INTERFACE_ATTR_INGRESS_ACL, "RIF Ingress ACL"},
            {SAI_ROUTER_INTERFACE_ATTR_EGRESS_ACL, "RIF Egress ACL"},
            {SAI_ROUTER_INTERFACE_ATTR_V4_MCAST_ENABLE, "RIF V4 Mcast Enable"},
            {SAI_ROUTER_INTERFACE_ATTR_V6_MCAST_ENABLE, "RIF V6 Mcast Enable"},
        };

    try {
        const auto& map_kv = _rif_attr_id_2_str_map.find (attr_id);

        if (map_kv != _rif_attr_id_2_str_map.end ()) {
            return map_kv->second;
        }
    }
    catch (...) {
        return "INVALID/UNKNOWN";
    }

    return "INVALID/UNKNOWN";
}

static void sai_test_rif_attr_value_fill (sai_attribute_t *p_attr,
                                          unsigned long attr_val,
                                          const char *mac_str)
{
    uint8_t sai_mac [6];

    switch (p_attr->id) {
        case SAI_ROUTER_INTERFACE_ATTR_TYPE:
            p_attr->value.s32 = attr_val;

            printf ("Value: %d\r\n", p_attr->value.s32);
            break;

        case SAI_ROUTER_INTERFACE_ATTR_VIRTUAL_ROUTER_ID:
        case SAI_ROUTER_INTERFACE_ATTR_PORT_ID:
            p_attr->value.oid = attr_val;

            printf ("Value: 0x%" PRIx64 "\r\n", p_attr->value.oid);
            break;
        case SAI_ROUTER_INTERFACE_ATTR_MTU:
            p_attr->value.u32 = attr_val;

            printf ("Value: %d\r\n", p_attr->value.u32);
            break;

        case SAI_ROUTER_INTERFACE_ATTR_VLAN_ID:
            p_attr->value.u16 = attr_val;

            printf ("Value: %d\r\n", p_attr->value.u16);
            break;

        case SAI_ROUTER_INTERFACE_ATTR_ADMIN_V4_STATE:
        case SAI_ROUTER_INTERFACE_ATTR_ADMIN_V6_STATE:
            p_attr->value.booldata = attr_val;

            printf ("Value: %d\r\n", p_attr->value.booldata);
            break;

        case SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS:
            saiL3Test ::sai_test_router_mac_str_to_bytes_get (mac_str, sai_mac);

            memcpy (p_attr->value.mac, sai_mac, sizeof (sai_mac_t));

            break;

        default:
            p_attr->value.u64 = attr_val;

            printf ("Value: %d\r\n", (int)p_attr->value.u64);
            break;
    }
}

static void sai_test_rif_attr_value_print (sai_attribute_t *p_attr)
{
    unsigned int idx = 0;

    switch (p_attr->id) {
        case SAI_ROUTER_INTERFACE_ATTR_TYPE:
            printf ("RIF Type: %d.\r\n", p_attr->value.s32);
            break;

        case SAI_ROUTER_INTERFACE_ATTR_VIRTUAL_ROUTER_ID:
            printf ("RIF VRF: 0x%" PRIx64 ".\r\n", p_attr->value.oid);
            break;

        case SAI_ROUTER_INTERFACE_ATTR_PORT_ID:
            printf ("RIF Port ID: 0x%" PRIx64 ".\r\n", p_attr->value.oid);
            break;

        case SAI_ROUTER_INTERFACE_ATTR_VLAN_ID:
            printf ("RIF VLAN: %d\r\n", p_attr->value.u16);
            break;

        case SAI_ROUTER_INTERFACE_ATTR_MTU:
            printf ("RIF MTU: %d.\r\n", p_attr->value.u32);
            break;

        case SAI_ROUTER_INTERFACE_ATTR_ADMIN_V4_STATE:
            printf ("RIF V4 Admin State: %d\r\n", p_attr->value.booldata);
            break;

        case SAI_ROUTER_INTERFACE_ATTR_ADMIN_V6_STATE:
            printf ("RIF V6 Admin State: %d\r\n", p_attr->value.booldata);
            break;

        case SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS:
            printf ("RIF SAI MAC: ");

            for (idx = 0; idx < 6; idx++) {
                if (idx != 5) {
                    printf ("%x:", p_attr->value.mac [idx]);
                } else {
                    printf ("%x\r\n", p_attr->value.mac [idx]);
                }
            }

            break;

        default:
            printf ("%d is an Invalid RIF Attribute ID.\r\n", p_attr->id);
            break;
    }
}

/*
 * p_rif_id     - [out] pointer to RIF ID generated by the API.
 * attr_count  - [in] number of RIF attributes passed.
 *               For each attribute, {id, value} is passed.
 *
 * For attr-count = 2,
 * sai_test_rif_create (p_rif_id, 2, id_0, val_0, id_1, val_1)
 */
sai_status_t saiL3Test ::sai_test_rif_create (
sai_object_id_t *p_rif_id, unsigned int attr_count, ...)
{
    sai_status_t     sai_rc = SAI_STATUS_SUCCESS;
    va_list          ap;
    unsigned int     ap_idx = 0;
    sai_attribute_t *p_attr_list = NULL;
    unsigned int     attr_id = 0;
    unsigned long    val = 0;
    char             default_mac[SAI_MAX_MAC_LENGTH] = {0};
    char            *mac_str = NULL;
    //sai_vlan_port_t  vlan_port[1];

    p_attr_list
        = (sai_attribute_t *) calloc (attr_count, sizeof (sai_attribute_t));

    printf ("Testing RIF Create API with attribute count: %d\r\n", attr_count);

    if (!p_attr_list) {
        printf ("Failed to allocate memory for attribute list.\r\n");

        return SAI_STATUS_FAILURE;
    }

    va_start (ap, attr_count);

    for (ap_idx = 0; ap_idx < attr_count; ap_idx++) {
        attr_id = va_arg (ap, unsigned int);
        p_attr_list [ap_idx].id = attr_id;

        printf ("Setting List index: %d with Attribute %s (ID = %d)\r\n",
                ap_idx, sai_test_rif_attr_id_to_name_get (attr_id),
                attr_id);

        if (SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS == attr_id) {
            mac_str = va_arg (ap, char*);
        } else {
            val = va_arg (ap, unsigned long);
            mac_str = default_mac; /* To avoid de-reference */
        }

        if (SAI_ROUTER_INTERFACE_ATTR_PORT_ID == attr_id) {
            /* Remove the port from default VLAN */
            if(sai_object_type_query(val) == SAI_OBJECT_TYPE_PORT) {
                sai_rc = sai_test_remove_port_from_vlan(p_sai_vlan_api_tbl,
                        default_vlan_id, val);
                if (sai_rc != SAI_STATUS_SUCCESS) {
                    printf("Failed to remove port from default vlan\r\n");
                }
            }
        }

        sai_test_rif_attr_value_fill (&p_attr_list [ap_idx], val, mac_str);
    }

    sai_rc = p_sai_rif_api_tbl->create_router_interface (p_rif_id, switch_id, attr_count,
                                                         p_attr_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI RIF Creation failed with error: %d\r\n", sai_rc);
    } else {
        printf ("SAI RIF Creation success, RIF ID: 0x%" PRIx64 "\r\n", *p_rif_id);
    }

    if (p_attr_list) {
        free (p_attr_list);
    }

    va_end (ap);
    return sai_rc;
}

sai_status_t saiL3Test ::sai_test_rif_remove (sai_object_id_t rif_id)
{
    int             rif_type;
    sai_status_t    sai_rc = SAI_STATUS_SUCCESS;
    sai_object_id_t port_id = 0;
    sai_attribute_t attr_list[1];

    sai_rc = sai_test_rif_attr_get (rif_id, attr_list, 1,
                                    SAI_ROUTER_INTERFACE_ATTR_TYPE);
    rif_type = attr_list [0].value.s32;

    if (rif_type == SAI_ROUTER_INTERFACE_TYPE_PORT) {

         sai_rc = sai_test_rif_attr_get (rif_id, attr_list, 1,
                                         SAI_ROUTER_INTERFACE_ATTR_PORT_ID);
         port_id = attr_list [0].value.oid;
    }
    sai_rc = p_sai_rif_api_tbl->remove_router_interface (rif_id);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI RIF Removal failed with error: %d\r\n", sai_rc);
    } else {
        printf ("SAI RIF Removal success for RIF ID: 0x%" PRIx64 "\r\n", rif_id);
    }

    if((rif_type == SAI_ROUTER_INTERFACE_TYPE_PORT) &&
       (sai_object_type_query(port_id) == SAI_OBJECT_TYPE_PORT)) {
        /* Add the port back to default VLAN */
        printf ("Testing Adding ports in/from Vlan \r\n");
        sai_rc = sai_test_add_port_to_vlan(switch_id, p_sai_vlan_api_tbl,
                default_vlan_id, port_id);
        if(sai_rc != SAI_STATUS_SUCCESS) {
            printf("Failed to add port to default vlan\r\n");
        }
    }

    return sai_rc;
}

sai_status_t saiL3Test ::sai_test_rif_attr_set (
sai_object_id_t rif_id, unsigned int attr_id, unsigned long val,
const char *mac_str)
{
    sai_status_t    sai_rc = SAI_STATUS_SUCCESS;
    sai_attribute_t attr;

    attr.id = attr_id;

    printf ("Setting Attribute %s (ID = %d) for RIF 0x%" PRIx64 ".\r\n",
            sai_test_rif_attr_id_to_name_get (attr_id), attr_id, rif_id);

    sai_test_rif_attr_value_fill (&attr, val, mac_str);

    sai_rc = p_sai_rif_api_tbl->set_router_interface_attribute (rif_id, &attr);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI RIF attribute set failed with error: %d\r\n", sai_rc);
    } else {
        printf ("SAI RIF attribute set success.\r\n");
    }

    return sai_rc;
}

/*
 * rif_id      - [in] RIF ID.
 * p_attr_list - [in, out] pointer to attribute list.
 * attr_count  - [in] number of attributes to get.
 *                    For each attribute, SAI attribute id is passed.
 *
 * For attr_count = 2,
 * sai_test_rif_attr_get (rif_id, 2, p_attr_list, id_0, id_1)
 */
sai_status_t saiL3Test ::sai_test_rif_attr_get (
sai_object_id_t rif_id, sai_attribute_t *p_attr_list,
unsigned int attr_count,...)
{
    sai_status_t sai_rc = SAI_STATUS_SUCCESS;
    va_list      ap;
    unsigned int ap_idx = 0;
    unsigned int attr_id = 0;

    printf ("Testing RIF GET API for RIF ID: 0x%" PRIx64 " with attribute count:"
            " %d\r\n", rif_id, attr_count);

    va_start (ap, attr_count);

    /* Fill in the RIF attribute IDs for get */
    for (ap_idx = 0; ap_idx < attr_count; ap_idx++) {
        attr_id = va_arg (ap, unsigned int);
        p_attr_list [ap_idx].id = attr_id;

        printf ("Setting List index: %d with Attribute %s (ID = %d)\r\n",
                ap_idx, sai_test_rif_attr_id_to_name_get (attr_id), attr_id);
    }

    sai_rc =
        p_sai_rif_api_tbl->get_router_interface_attribute (rif_id, attr_count,
                                                           p_attr_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("RIF Get API failed with error: %d\r\n", sai_rc);
    } else {
        printf ("RIF Get API success, RIF ID: 0x%" PRIx64 "\r\n", rif_id);

        printf ("\r\n##### RIF Attributes Details #####\r\n");

        for (ap_idx = 0; ap_idx < attr_count; ap_idx++) {
            printf ("RIF attribute list idx: %d, SAI Attribute ID: %d.\r\n",
                    ap_idx, p_attr_list [ap_idx].id);

            sai_test_rif_attr_value_print (&p_attr_list [ap_idx]);
        }
    }

    va_end (ap);
    return sai_rc;
}

static uint32_t sai_test_ip_v4_prefix_len_to_mask (unsigned int prefix_len)
{
    uint32_t mask_val = 0;
    const unsigned int NBBY = 8;

    if (prefix_len > (NBBY * sizeof(uint32_t))) {
        return 0;
    }

    mask_val = ((prefix_len)?((1 << ((NBBY * sizeof(uint32_t) - prefix_len))) -1)
                : 0xffffffff);

    return (uint32_t)~(mask_val);
}

static void sai_test_ip_v6_prefix_len_to_mask (uint8_t *mask_ptr, size_t mask_len,
                                               unsigned int prefix_len)
{
    unsigned int nbytes;
    static uint8_t mask_val[8] = { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};
    const unsigned int NBBY = 8;

    if ((!mask_ptr) || (!prefix_len)) {
        return;
    }

    nbytes = prefix_len / NBBY;
    memset(mask_ptr, 0xff, nbytes);
    mask_ptr += nbytes;
    mask_len -= nbytes;

    if (!mask_len) {
        return;
    }

    *mask_ptr++ = mask_val[prefix_len % NBBY];
    --mask_len;
    memset(mask_ptr, 0, mask_len);
}

static sai_status_t sai_test_route_entry_fill (
sai_object_id_t vrf_id, unsigned int rt_af, const char *ip_str,
unsigned int rt_prefix_len, sai_route_entry_t *p_uc_route)
{
    const unsigned int IPV6_ADDR_BYTE_LEN = 16;

    p_uc_route->vr_id = vrf_id;
    p_uc_route->destination.addr_family = (sai_ip_addr_family_t) rt_af;

    if (rt_af == SAI_IP_ADDR_FAMILY_IPV4) {
        inet_pton (AF_INET, ip_str, (void *)&p_uc_route->destination.addr.ip4);

        p_uc_route->destination.mask.ip4 =
            sai_test_ip_v4_prefix_len_to_mask (rt_prefix_len);
    } else if (rt_af == SAI_IP_ADDR_FAMILY_IPV6) {
        inet_pton (AF_INET6, ip_str, (void *)p_uc_route->destination.addr.ip6);

        sai_test_ip_v6_prefix_len_to_mask (p_uc_route->destination.mask.ip6,
                                           IPV6_ADDR_BYTE_LEN, rt_prefix_len);
    }

    return SAI_STATUS_SUCCESS;
}

static const char* sai_test_route_attr_id_to_name_get (unsigned int attr_id)
{
    if (SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION == attr_id) {
        return "ROUTE PACKET ACTION";
    } else if (SAI_ROUTE_ENTRY_ATTR_TRAP_PRIORITY == attr_id) {
        return "ROUTE TRAP PRIORITY";
    } else if (SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID == attr_id) {
        return "ROUTE NEXT-HOP ID OBJECT";
    } else if (SAI_ROUTE_ENTRY_ATTR_META_DATA == attr_id) {
        return "ROUTE META DATA";
    } else {
        return "INVALID/UNKNOWN";
    }
}

static void sai_test_route_attr_value_fill (sai_attribute_t *p_attr,
                                            unsigned long attr_val)
{
    switch (p_attr->id) {
        case SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION:
            p_attr->value.s32 = attr_val;

            printf ("Value: %d\r\n", p_attr->value.s32);
            break;

        case SAI_ROUTE_ENTRY_ATTR_TRAP_PRIORITY:
            p_attr->value.u8 = attr_val;

            printf ("Value: %d\r\n", p_attr->value.u8);
            break;

        case SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID:
            p_attr->value.oid = attr_val;

            printf ("Value: 0x%" PRIx64 "\r\n", p_attr->value.oid);
            break;

        case SAI_ROUTE_ENTRY_ATTR_META_DATA:
            p_attr->value.u32 = attr_val;

            printf ("Value: %d\r\n", p_attr->value.u32);
            break;

        default:
            p_attr->value.u64 = attr_val;

            printf ("Value: %d\r\n", (int)p_attr->value.u64);
            break;
    }
}

static void sai_test_route_attr_value_print (sai_attribute_t *p_attr)
{
    switch (p_attr->id) {
        case SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION:
            printf ("Packet action: %d\r\n", p_attr->value.s32);
            break;

        case SAI_ROUTE_ENTRY_ATTR_TRAP_PRIORITY:
            printf ("Trap priority: %d\r\n", p_attr->value.u8);
            break;

        case SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID:
            printf ("Next Hop object ID: 0x%" PRIx64 "\r\n", p_attr->value.oid);
            break;

        case SAI_ROUTE_ENTRY_ATTR_META_DATA:
            printf ("Route Meta Data: %d\r\n", p_attr->value.u32);
            break;

        default:
            printf ("%d is an Invalid Route Attribute ID.\r\n", p_attr->id);
            break;
    }
}

/*
 * vrf         - [in] VRF ID.
 * ip_family   - [in] Ipv4/v6 address family.
 * ip_str      - [in] ip prefix string.
 * prefix_len  - [in] prefix length.
 * attr_count  - [in] number of attributes set.
 *
 * For attr_count = 2,
 * sai_test_route_attr_get (vrf, ip_family, ip_str, prefix_len, 2, p_attr_list,
 *                          id_0, id_1)
 */
sai_status_t saiL3Test ::sai_test_route_create (sai_object_id_t vrf,
                                                unsigned int ip_family,
                                                const char *ip_str,
                                                unsigned int prefix_len,
                                                unsigned int attr_count, ...)
{
    sai_status_t               sai_rc = SAI_STATUS_SUCCESS;
    sai_route_entry_t  uc_route_entry;
    va_list                    ap;
    unsigned int               ap_idx = 0;
    sai_attribute_t           *p_attr_list = NULL;
    unsigned int               attr_id = 0;
    unsigned long              val = 0;

    memset (&uc_route_entry, 0, sizeof (uc_route_entry));

    sai_rc = sai_test_route_entry_fill (vrf, ip_family, ip_str, prefix_len,
                                        &uc_route_entry);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("Failed to fill UC ROUTE entry.\r\n");

        return sai_rc;
    }

    printf ("Testing ROUTE Create API with attribute count: %d\r\n",
            attr_count);

    p_attr_list
        = (sai_attribute_t *) calloc (attr_count, sizeof (sai_attribute_t));

    if (!p_attr_list) {
        printf ("Failed to allocate memory for attribute list.\r\n");

        return SAI_STATUS_FAILURE;
    }

    va_start (ap, attr_count);

    for (ap_idx = 0; ap_idx < attr_count; ap_idx++) {
        attr_id = va_arg (ap, unsigned int);
        p_attr_list [ap_idx].id = attr_id;

        printf ("Setting List index: %d with Attribute %s (ID = %d)\r\n",
                ap_idx, sai_test_route_attr_id_to_name_get (attr_id), attr_id);

        val = va_arg (ap, unsigned long);

        sai_test_route_attr_value_fill (&p_attr_list [ap_idx], val);
    }

    sai_rc = p_sai_route_api_tbl->create_route_entry(&uc_route_entry, attr_count,
                                                p_attr_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI ROUTE Creation failed with error: %d\r\n", sai_rc);
    } else {
        printf ("SAI ROUTE Creation success.\r\n");
    }

    if (p_attr_list) {
        free (p_attr_list);
    }

    va_end (ap);
    return sai_rc;
}

sai_status_t saiL3Test ::sai_test_route_remove (sai_object_id_t vrf,
                                                unsigned int ip_family,
                                                const char *ip_str,
                                                unsigned int prefix_len)
{
    sai_status_t              sai_rc = SAI_STATUS_SUCCESS;
    sai_route_entry_t uc_route_entry;

    memset (&uc_route_entry, 0, sizeof (uc_route_entry));

    sai_rc = sai_test_route_entry_fill (vrf, ip_family, ip_str, prefix_len,
                                        &uc_route_entry);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("Failed to fill UC ROUTE entry.\r\n");

        return sai_rc;
    }

    sai_rc = p_sai_route_api_tbl->remove_route_entry(&uc_route_entry);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI Route remove failed with error: %d\r\n", sai_rc);
    } else {
        printf ("SAI Route remove success.\r\n");
    }

    return sai_rc;
}

sai_status_t saiL3Test ::sai_test_route_attr_set (sai_object_id_t vrf,
                                                  unsigned int ip_family,
                                                  const char *ip_str,
                                                  unsigned int prefix_len,
                                                  unsigned int attr_id,
                                                  unsigned long value)
{
    sai_status_t              sai_rc = SAI_STATUS_SUCCESS;
    sai_route_entry_t uc_route_entry;
    sai_attribute_t           attr;

    memset (&uc_route_entry, 0, sizeof (uc_route_entry));

    sai_rc = sai_test_route_entry_fill (vrf, ip_family, ip_str, prefix_len,
                                        &uc_route_entry);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("Failed to fill UC ROUTE entry.\r\n");

        return sai_rc;
    }

    attr.id = attr_id;

    printf ("Setting Attribute %s (ID = %d).\r\n",
            sai_test_route_attr_id_to_name_get (attr_id), attr_id);

    sai_test_route_attr_value_fill (&attr, value);

    sai_rc = p_sai_route_api_tbl->set_route_entry_attribute(&uc_route_entry, &attr);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI ROUTE attribute set failed with error: %d\r\n",
                sai_rc);
    } else {
        printf ("SAI ROUTE attribute set success.\r\n");
    }

    return sai_rc;
}

/*
 * vrf         - [in] VRF ID.
 * ip_family   - [in] Ipv4/v6 address family.
 * ip_str      - [in] ip prefix string.
 * prefix_len  - [in] prefix length.
 * p_attr_list - [in, out] pointer to attribute list.
 * attr_count  - [in] number of attributes to get.
 *                    For each attribute, SAI attribute id is passed.
 *
 * For attr_count = 2,
 * sai_test_route_attr_get (vrf, ip_family, ip_str, prefix_len, 2, p_attr_list,
 *                          id_0, id_1)
 */
sai_status_t saiL3Test ::sai_test_route_attr_get (sai_object_id_t vrf,
                                                  unsigned int ip_family,
                                                  const char *ip_str,
                                                  unsigned int prefix_len,
                                                  sai_attribute_t *p_attr_list,
                                                  unsigned int attr_count, ...)
{
    sai_status_t              sai_rc = SAI_STATUS_SUCCESS;
    sai_route_entry_t uc_route_entry;
    va_list                   ap;
    unsigned int              ap_idx = 0;
    unsigned int              attr_id = 0;

    memset (&uc_route_entry, 0, sizeof (uc_route_entry));

    printf ("Testing Route GET API for VRF: 0x%" PRIx64 ", ip_family: %u, "
            "ip_str: %s, prefix_len: %u with attribute count: %u\r\n", vrf,
            ip_family, ip_str, prefix_len, attr_count);

    sai_rc = sai_test_route_entry_fill (vrf, ip_family, ip_str, prefix_len,
                                        &uc_route_entry);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("Failed to fill UC ROUTE entry.\r\n");

        return sai_rc;
    }

    va_start (ap, attr_count);

    /* Fill in the Route attribute IDs for get */
    for (ap_idx = 0; ap_idx < attr_count; ap_idx++) {
        attr_id = va_arg (ap, unsigned int);
        p_attr_list [ap_idx].id = attr_id;

        printf ("Setting List index: %d with Attribute %s (ID = %d)\r\n",
                ap_idx, sai_test_route_attr_id_to_name_get (attr_id), attr_id);
    }

    sai_rc = p_sai_route_api_tbl->get_route_entry_attribute(&uc_route_entry,
                                                       attr_count, p_attr_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("Route Get API failed with error: %d\r\n", sai_rc);
    } else {
        printf ("Route Get API success.\r\n");

        printf ("\r\n##### Route Attributes Details #####\r\n");

        for (ap_idx = 0; ap_idx < attr_count; ap_idx++) {
            printf ("Route attribute list idx: %d, SAI Attribute ID: %d.\r\n",
                    ap_idx, p_attr_list [ap_idx].id);

            sai_test_route_attr_value_print (&p_attr_list [ap_idx]);
        }
    }

    va_end (ap);
    return sai_rc;
}

const char* saiL3Test::sai_test_ip_addr_to_str (const sai_ip_address_t *p_ip_addr,
                                                char *p_buf, size_t len)
{
    if (p_ip_addr->addr_family == SAI_IP_ADDR_FAMILY_IPV4) {

        return (inet_ntop (AF_INET, (const void *) (&p_ip_addr->addr.ip4),
                           p_buf, len));

    } else {

        return (inet_ntop (AF_INET6, (const void *) (&p_ip_addr->addr.ip6),
                           p_buf, len));
    }
}

static void sai_test_nexthop_attr_value_pair_fill (unsigned int attr_count,
                                                   va_list *p_varg_list,
                                                   sai_attribute_t *p_attr_list)
{
    unsigned int     af;
    unsigned int     index;
    const char      *p_ip_str;
    sai_attribute_t *p_attr;

    for (index = 0; index < attr_count; index++) {

        p_attr = &p_attr_list [index];

        p_attr->id = va_arg ((*p_varg_list), unsigned int);

        switch (p_attr->id) {

            case SAI_NEXT_HOP_ATTR_TYPE:
                p_attr->value.s32 = va_arg ((*p_varg_list), unsigned int);
                printf ("Attr Index: %d, Set NH Type value: %d.\n",
                        index, p_attr->value.s32);
                break;

            case SAI_NEXT_HOP_ATTR_IP:
                af       = va_arg ((*p_varg_list), unsigned int);
                p_ip_str = va_arg ((*p_varg_list), const char *);

                if (af == SAI_IP_ADDR_FAMILY_IPV4) {

                    inet_pton (AF_INET, p_ip_str,
                               (void *)&p_attr->value.ipaddr.addr.ip4);
                } else {

                    inet_pton (AF_INET6, p_ip_str,
                               (void *)&p_attr->value.ipaddr.addr.ip6);
                }

                p_attr->value.ipaddr.addr_family = (sai_ip_addr_family_t) af;
                printf ("Attr Index: %d, Set NH IP Addr family: %d, "
                        "IP Addr: %s.\n", index, af, p_ip_str);
                break;

            case SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID:
                p_attr->value.oid = va_arg ((*p_varg_list), unsigned long);
                printf ("Attr Index: %d, Set NH RIF Id value: 0x%" PRIx64 ".\n",
                        index, p_attr->value.oid);
                break;

            case SAI_NEXT_HOP_ATTR_TUNNEL_ID:
                p_attr->value.oid = va_arg ((*p_varg_list), unsigned long);
                printf ("Attr Index: %d, Set NH Tunnel Id value: 0x%" PRIx64 ".\n",
                        index, p_attr->value.oid);
                break;

            default:
                p_attr->value.u64 = va_arg ((*p_varg_list), unsigned int);
                printf ("Attr Index: %d, Set unknown Attr Id: %d to value: %ld.\n",
                        index, p_attr->id, p_attr->value.u64);
                break;
        }
    }
}

/*
 * p_nh_id     - [out] pointer to Next Hop ID generated by the SAI API.
 * attr_count  - [in]  number of attributes passed.
 *               For each attribute, {id, value} is passed.
 *
 * For attr-count = 2,
 * sai_test_nexthop_create (p_nh_id, 2, id_0, val_0, id_1, val_1)
 */
sai_status_t saiL3Test::sai_test_nexthop_create (sai_object_id_t *p_nh_id,
                                                 unsigned int attr_count, ...)
{
    sai_status_t     sai_rc;
    sai_attribute_t *p_attr_list = NULL;
    va_list          varg_list;

    p_attr_list = (sai_attribute_t *) calloc (attr_count,
                                              sizeof (sai_attribute_t));
    if (p_attr_list == NULL) {

        printf ("%s(): Memory alloc failed for attribute list.\n", __FUNCTION__);

        return SAI_STATUS_NO_MEMORY;
    }

    va_start (varg_list, attr_count);

    sai_test_nexthop_attr_value_pair_fill (attr_count, &varg_list,
                                           p_attr_list);

    va_end (varg_list);

    sai_rc = p_sai_nh_api_tbl->create_next_hop (p_nh_id, switch_id, attr_count,
                                                p_attr_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {

        printf ("SAI Next Hop Creation failed with error: %d.\n", sai_rc);

    } else {

        printf ("SAI Next Hop Creation success, NH Id: 0x%" PRIx64 ".\n", (*p_nh_id));
    }

    free (p_attr_list);

    return sai_rc;
}

sai_status_t saiL3Test::sai_test_nexthop_remove (sai_object_id_t nh_id)
{
    sai_status_t sai_rc;

    sai_rc = saiL3Test::p_sai_nh_api_tbl->remove_next_hop (nh_id);

    if (sai_rc != SAI_STATUS_SUCCESS) {

        printf ("SAI Next Hop removal failed with error: %d.\n", sai_rc);

    } else {

        printf ("SAI Next Hop removal success for NH ID: 0x%" PRIx64 ".\n", nh_id);
    }

    return sai_rc;
}

static void sai_test_nexthop_attr_list_print (unsigned int attr_count,
                                              sai_attribute_t *p_attr_list)
{
    unsigned int     attr_index;
    sai_attribute_t *p_attr;
    char             buf[256];

    printf ("Printing SAI Next Hop attribute list..\n");
    for (attr_index = 0; attr_index < attr_count; attr_index++)
    {
        p_attr = &p_attr_list [attr_index];

        switch (p_attr->id) {

            case SAI_NEXT_HOP_ATTR_IP:
                printf ("Index: %d, Next Hop IP Addr: %s.\n", attr_index,
                        saiL3Test::sai_test_ip_addr_to_str (&p_attr->value.ipaddr,
                        buf, sizeof (buf)));
                break;

            case SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID:
                printf ("Index: %d, Next Hop RIF Id: 0x%" PRIx64 ".\n", attr_index,
                        p_attr->value.oid);
                break;

            case SAI_NEXT_HOP_ATTR_TYPE:
                printf ("Index: %d, Next Hop Type: %d.\n", attr_index,
                        p_attr->value.s32);

                break;

            default:
                printf ("Index: %d, Next Hop unknown Attr Id: %d, Value: %ld.\n",
                        attr_index, p_attr->id, p_attr->value.u64);
                break;
        }
    }
}

/*
 * nh_id       - [in] Next Hop ID.
 * p_attr_list - [in, out] pointer to attribute list.
 * attr_count  - [in] number of attributes to get.
 *                    For each attribute, SAI attribute id is passed.
 *
 * For attr_count = 2,
 * sai_test_nexthop_attr_get (nh_id, p_attr_list, 2, id_0, id_1)
 */
sai_status_t saiL3Test::sai_test_nexthop_attr_get (sai_object_id_t nh_id,
                                                   sai_attribute_t *p_attr_list,
                                                   unsigned int attr_count, ...)
{
    sai_status_t         sai_rc;
    va_list              varg_list;
    unsigned int         index;

    va_start (varg_list, attr_count);

    for (index = 0; index < attr_count; index++) {

        p_attr_list [index].id = va_arg (varg_list, unsigned int);
    }

    va_end (varg_list);

    sai_rc = p_sai_nh_api_tbl->get_next_hop_attribute (nh_id, attr_count,
                                                       p_attr_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {

        printf ("SAI Next Hop Get attribute failed with error: %d.\n", sai_rc);

    } else {

        printf ("SAI Next Hop Get attribute success.\n");

        sai_test_nexthop_attr_list_print (attr_count, p_attr_list);
    }

    return sai_rc;
}

static void sai_test_neighbor_attr_value_pair_fill (sai_attribute_t *p_attr,
                                                    sai_attr_id_t attr_id,
                                                    unsigned int attr_value_int,
                                                    const char *attr_value_str)
{
    uint8_t   sai_mac [6];

    p_attr->id = attr_id;

    switch (p_attr->id) {

        case SAI_NEIGHBOR_ENTRY_ATTR_DST_MAC_ADDRESS:
            saiL3Test::sai_test_router_mac_str_to_bytes_get (attr_value_str,
                                                             sai_mac);

            memcpy (p_attr->value.mac, sai_mac, sizeof (sai_mac_t));
            break;

        case SAI_NEIGHBOR_ENTRY_ATTR_PACKET_ACTION:
            p_attr->value.s32 = attr_value_int;
            printf ("Set Neighbor Pkt action value: %d.\n", p_attr->value.s32);
            break;

        case SAI_NEIGHBOR_ENTRY_ATTR_NO_HOST_ROUTE:
            p_attr->value.booldata = attr_value_int;
            printf ("Set Neighbor No Host Route value: %d.\n",
                    p_attr->value.booldata);
            break;

        case SAI_NEIGHBOR_ENTRY_ATTR_META_DATA:
            p_attr->value.u32 = attr_value_int;
            printf ("Set Neighbor Meta Data value: %d.\n",
                    p_attr->value.u32);
            break;

        default:
            p_attr->value.u64 = attr_value_int;
            printf ("Set unknown Attr Id: %d to value: %ld.\n", p_attr->id,
                    p_attr->value.u64);
            break;
    }
}

/*
 * rif_id      - [in] RIF ID.
 * addr_family - [in] Ipv4/v6 address family.
 * ip_str      - [in] ip addr string.
 * attr_count  - [in]  number of attributes passed.
 *               For each attribute, {id, value} is passed.
 *
 * For attr_count = 2,
 * sai_test_neighbor_create (rif_id, ip_family, ip_str, 2, id_0, val_0,
 *                           id_1, val_1)
 */
sai_status_t saiL3Test::sai_test_neighbor_create (
                                             sai_object_id_t rif_id,
                                             sai_ip_addr_family_t addr_family,
                                             const char *ip_str,
                                             unsigned int attr_count, ...)
{
    sai_status_t         sai_rc;
    sai_attribute_t     *p_attr_list = NULL;
    sai_attribute_t     *p_attr = NULL;
    va_list              varg_list;
    sai_neighbor_entry_t neighbor_entry;
    unsigned int         index;
    const char          *p_mac_str;
    unsigned int         attr_val;

    neighbor_entry.rif_id                 = rif_id;
    neighbor_entry.ip_address.addr_family = addr_family;

    if (addr_family == SAI_IP_ADDR_FAMILY_IPV4) {

        inet_pton (AF_INET, ip_str, (void *) &neighbor_entry.ip_address.addr.ip4);

    } else {

        inet_pton (AF_INET6, ip_str, (void *) &neighbor_entry.ip_address.addr.ip6);
    }

    printf ("SAI Neighbor entry, RIF Id: 0x%" PRIx64 ", IP addr: %s.\n",
            rif_id, ip_str);

    p_attr_list = (sai_attribute_t *) calloc (attr_count,
                                              sizeof (sai_attribute_t));
    if (p_attr_list == NULL) {

        printf ("%s(): Memory alloc failed for attribute list.\n", __FUNCTION__);

        return SAI_STATUS_NO_MEMORY;
    }

    va_start (varg_list, attr_count);

    for (index = 0; index < attr_count; index++) {

        p_mac_str = NULL;
        attr_val  = 0;

        p_attr = &p_attr_list [index];

        p_attr->id = va_arg (varg_list, unsigned int);

        if (p_attr->id == SAI_NEIGHBOR_ENTRY_ATTR_DST_MAC_ADDRESS) {

            p_mac_str = va_arg (varg_list, const char *);

        } else {

            attr_val = va_arg (varg_list, unsigned int);
        }

        printf ("Attr Index: %d, ", index);

        sai_test_neighbor_attr_value_pair_fill (p_attr, p_attr->id, attr_val,
                                                p_mac_str);
    }

    va_end (varg_list);

    sai_rc = p_sai_nbr_api_tbl->create_neighbor_entry (
                                (const sai_neighbor_entry_t *) &neighbor_entry,
                                attr_count, p_attr_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {

        printf ("SAI Neighbor Creation failed with error: %d.\n", sai_rc);

    } else {

        printf ("SAI Neighbor Creation success.\n");
    }

    free (p_attr_list);

    return sai_rc;
}

sai_status_t saiL3Test::sai_test_neighbor_remove (
                                             sai_object_id_t rif_id,
                                             sai_ip_addr_family_t addr_family,
                                             const char *ip_str)
{
    sai_status_t         sai_rc;
    sai_neighbor_entry_t neighbor_entry;

    neighbor_entry.ip_address.addr_family = addr_family;

    if (addr_family == SAI_IP_ADDR_FAMILY_IPV4) {

        inet_pton (AF_INET, ip_str, (void *) &neighbor_entry.ip_address.addr.ip4);

    } else {
        inet_pton (AF_INET6, ip_str, (void *) &neighbor_entry.ip_address.addr.ip6);
    }

    neighbor_entry.rif_id = rif_id;

    sai_rc = saiL3Test::p_sai_nbr_api_tbl->remove_neighbor_entry (
                               (const sai_neighbor_entry_t *) &neighbor_entry);

    if (sai_rc != SAI_STATUS_SUCCESS) {

        printf ("SAI Neighbor remove failed with error: %d.\n", sai_rc);

    } else {

        printf ("SAI Neighbor remove success.\n");
    }

    return sai_rc;
}

sai_status_t saiL3Test::sai_test_neighbor_attr_set (
                                             sai_object_id_t rif_id,
                                             sai_ip_addr_family_t addr_family,
                                             const char *ip_str,
                                             sai_attr_id_t attr_id,
                                             unsigned int int_attr_value,
                                             const char *p_str_attr_value)
{
    sai_status_t         sai_rc;
    sai_attribute_t      attr;
    sai_neighbor_entry_t neighbor_entry;

    neighbor_entry.rif_id                 = rif_id;
    neighbor_entry.ip_address.addr_family = addr_family;

    if (addr_family == SAI_IP_ADDR_FAMILY_IPV4) {

        inet_pton (AF_INET, ip_str, (void *) &neighbor_entry.ip_address.addr.ip4);

    } else {

        inet_pton (AF_INET6, ip_str, (void *) &neighbor_entry.ip_address.addr.ip6);
    }

    sai_test_neighbor_attr_value_pair_fill (&attr, attr_id, int_attr_value,
                                            p_str_attr_value);

    sai_rc = p_sai_nbr_api_tbl->set_neighbor_entry_attribute(
                                (const sai_neighbor_entry_t *) &neighbor_entry,
                                &attr);

    if (sai_rc != SAI_STATUS_SUCCESS) {

        printf ("SAI Neighbor Set attribute failed with error: %d.\n", sai_rc);

    } else {

        printf ("SAI Neighbor Set attribute success.\n");
    }

    return sai_rc;
}

static void sai_test_neighbor_attr_list_print (unsigned int attr_count,
                                               sai_attribute_t *p_attr_list)
{
    unsigned int     attr_index;
    unsigned int     idx;
    sai_attribute_t *p_attr;

    printf ("Printing SAI Neighbor attribute list..\n");
    for (attr_index = 0; attr_index < attr_count; attr_index++)
    {
        p_attr = &p_attr_list [attr_index];

        switch (p_attr->id) {

            case SAI_NEIGHBOR_ENTRY_ATTR_DST_MAC_ADDRESS:
                printf ("Index: %d, Neighbor DST MAC: ", attr_index);

                for (idx = 0; idx < 6; idx++) {
                    if (idx != 5) {
                        printf ("%x:", p_attr->value.mac [idx]);
                    } else {
                        printf ("%x.\n", p_attr->value.mac [idx]);
                    }
                }
                break;

            case SAI_NEIGHBOR_ENTRY_ATTR_PACKET_ACTION:
                printf ("Index: %d, Neighbor Pkt action: %d.\n",
                        attr_index, p_attr->value.s32);
                break;

            case SAI_NEIGHBOR_ENTRY_ATTR_NO_HOST_ROUTE:
                printf ("Index: %d, Neighbor No Host Route: %d.\n",
                        attr_index, p_attr->value.booldata);
                break;

            case SAI_NEIGHBOR_ENTRY_ATTR_META_DATA:
                printf ("Index: %d, Neighbor Meta Data: %d.\n",
                        attr_index, p_attr->value.u32);
                break;

            default:
                printf ("Index: %d, Neighbor unknown Attr Id: %d, Value: %ld.\n",
                        attr_index, p_attr->id, p_attr->value.u64);
                break;
        }
    }
}

/*
 * rif_id      - [in] RIF ID.
 * addr_family - [in] Ipv4/v6 address family.
 * ip_str      - [in] ip addr string.
 * p_attr_list - [in, out] pointer to attribute list.
 * attr_count  - [in] number of attributes to get.
 *                    For each attribute, SAI attribute id is passed.
 *
 * For attr_count = 2,
 * sai_test_neighbor_attr_get (rif_id, ip_family, ip_str, p_attr_list, 2, id_0,
 *                             id_1)
 */
sai_status_t saiL3Test::sai_test_neighbor_attr_get (
                                             sai_object_id_t rif_id,
                                             sai_ip_addr_family_t addr_family,
                                             const char *ip_str,
                                             sai_attribute_t *p_attr_list,
                                             unsigned int attr_count, ...)
{
    sai_status_t         sai_rc;
    sai_neighbor_entry_t neighbor_entry;
    va_list              varg_list;
    unsigned int         index;

    neighbor_entry.rif_id                 = rif_id;
    neighbor_entry.ip_address.addr_family = addr_family;

    if (addr_family == SAI_IP_ADDR_FAMILY_IPV4) {

        inet_pton (AF_INET, ip_str, (void *) &neighbor_entry.ip_address.addr.ip4);

    } else {

        inet_pton (AF_INET6, ip_str, (void *) &neighbor_entry.ip_address.addr.ip6);
    }

    va_start (varg_list, attr_count);

    for (index = 0; index < attr_count; index++) {

        p_attr_list [index].id = va_arg (varg_list, unsigned int);
    }

    va_end (varg_list);

    sai_rc = p_sai_nbr_api_tbl->get_neighbor_entry_attribute (
                                (const sai_neighbor_entry_t *) &neighbor_entry,
                                attr_count, p_attr_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {

        printf ("SAI Neighbor Get Attribute failed with error: %d.\n", sai_rc);

    } else {

        printf ("SAI Neighbor Get Attribute success.\n");

        sai_test_neighbor_attr_list_print (attr_count, p_attr_list);
    }

    return sai_rc;
}

/*
 * p_group_id  - [out] pointer to Next Hop Group ID generated by the SAI API.
 * type        - [in]  Next Hop Group Type.
 */
sai_status_t saiL3Test::sai_test_nh_group_create_no_nh_list (
                                           sai_object_id_t *p_group_id,
                                           sai_next_hop_group_type_t type)
{
    sai_status_t    sai_rc;
    sai_attribute_t attr;

    attr.id        = SAI_NEXT_HOP_GROUP_ATTR_TYPE;
    attr.value.s32 = type;

    sai_rc = p_sai_nh_grp_api_tbl->
        create_next_hop_group (p_group_id, switch_id, 1, &attr);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI NH Group Creation failed with error: %d.\n", sai_rc);
    }
    else {
        printf ("SAI NH Group Creation success, Group Id: 0x%" PRIx64 ".\n",
                (*p_group_id));
    }

    return sai_rc;
}

/*
 * p_group_id  - [out] pointer to Next Hop Group ID generated by the SAI API.
 * p_nh_list   - [in]  pointer to the next hop list.
 * attr_count  - [in]  number of attributes passed.
 *               For each attribute, {id, value} is passed. For
 *               SAI_NEXT_HOP_GROUP_ATTR_NEXT_HOP_MEMBER_LIST next-hop count
 *               alone is passed. next-hop list is assigned from the input arg.
 *               Though SAI_NEXT_HOP_GROUP_ATTR_NEXT_HOP_MEMBER_LIST represents
 *               the list NH Group Member Ids (and not Next Hop Ids), but in the
 *               unit test code, this will be used to represent the Next Hop Ids
 *
 * For attr-count = 2,
 * sai_test_nh_group_create (p_group_id, p_nh_list, 2, id_0, val_0, id_1, val_1)
 */
sai_status_t saiL3Test::sai_test_nh_group_create (
                                           sai_object_id_t *p_group_id,
                                           sai_object_id_t *p_nh_list,
                                           unsigned int attr_count, ...)
{
    sai_status_t       sai_rc;
    sai_attribute_t   *p_attr_list = NULL;
    sai_attribute_t   *p_attr = NULL;
    sai_attr_id_t      attr_id;
    va_list            varg_list;
    unsigned int       index;
    unsigned int       attr_index = 0;
    unsigned int       nh_count = 0;

    p_attr_list = (sai_attribute_t *) calloc (attr_count,
                                              sizeof (sai_attribute_t));
    if (p_attr_list == NULL) {

        printf ("%s(): Memory alloc failed for attribute list.\n", __FUNCTION__);

        return SAI_STATUS_NO_MEMORY;
    }

    va_start (varg_list, attr_count);

    for (index = 0; index < attr_count; index++) {

        p_attr = &p_attr_list [attr_index];

        attr_id = va_arg (varg_list, unsigned int);

        switch (attr_id) {

            case SAI_NEXT_HOP_GROUP_ATTR_TYPE:
                p_attr->id = attr_id;
                p_attr->value.s32 = va_arg (varg_list, unsigned int);

                printf ("Attr Index: %d, Set NH Group Type to value: %d.\n",
                        attr_index, p_attr->value.s32);
                attr_index++;
                break;

            case SAI_NEXT_HOP_GROUP_ATTR_NEXT_HOP_MEMBER_LIST:
                /*
                 * NH list is not a valid attribute for Group Create.
                 * So skip this attribute after obtaining the nh_count.
                 */
                nh_count = va_arg (varg_list, unsigned int);

                printf ("Attr Index: %d, Set NH Group NH list of count: %d.\n",
                        attr_index, nh_count);
                break;

            default:
                p_attr->value.u64 = va_arg (varg_list, unsigned int);
                printf ("Attr Index: %d, Set unknown Attr Id: %d to value: %ld.\n",
                        attr_index, p_attr->id, p_attr->value.u64);
                break;
        }
    }

    va_end (varg_list);

    sai_rc = p_sai_nh_grp_api_tbl->create_next_hop_group (p_group_id, switch_id,
                                                          attr_index, p_attr_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {

        printf ("SAI Next Hop Group Creation failed with error: %d.\n", sai_rc);

    } else {

        printf ("SAI Next Hop Group Creation success, Group Id: 0x%" PRIx64 ".\n",
                (*p_group_id));

        sai_rc = sai_test_add_nh_to_group (*p_group_id, nh_count, p_nh_list);

        if (sai_rc != SAI_STATUS_SUCCESS) {
            p_sai_nh_grp_api_tbl->remove_next_hop_group (*p_group_id);
        }
    }

    free (p_attr_list);

    return sai_rc;
}

sai_status_t saiL3Test::sai_test_nh_group_remove (sai_object_id_t group_id)
{
    sai_status_t     sai_rc;
    sai_object_id_t *nh_list;
    unsigned int     nh_count;

    sai_rc = sai_test_get_nh_count (group_id, &nh_count);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("Failed to get the NH count. Group ID: 0x%" PRIx64 ", Err: %d.",
                group_id, sai_rc);
        return sai_rc;
    }

    nh_list = (sai_object_id_t *) calloc (nh_count, sizeof (sai_object_id_t));

    if (nh_list == NULL) {
        return SAI_STATUS_NO_MEMORY;
    }

    sai_rc = sai_test_get_nh_list (group_id, &nh_count, nh_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("Failed to get the NH list. Group ID: 0x%" PRIx64 ", Err: %d.",
                group_id, sai_rc);
        free (nh_list);
        return sai_rc;
    }

    sai_rc = sai_test_remove_nh_from_group (group_id, nh_count, nh_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI Next Hop Group Member Remove failed with error: %d. "
                "Group Id: 0x%" PRIx64 "\n", sai_rc, group_id);

        free (nh_list);
        return sai_rc;
    }

    sai_rc = p_sai_nh_grp_api_tbl->remove_next_hop_group (group_id);

    if (sai_rc != SAI_STATUS_SUCCESS) {
        printf ("SAI Next Hop Group Remove failed with error: %d.\n", sai_rc);
    }
    else {
        printf ("SAI Next Hop Group Remove success. Group Id: 0x%" PRIx64 ".\n",
                group_id);
    }

    free (nh_list);
    return sai_rc;
}

static sai_status_t sai_test_convert_nh_grp_member_to_nh_id (
                                            sai_object_id_t  in_nh_group_id,
                                            unsigned int     attr_count,
                                            sai_attribute_t *p_attr_list)
{
    unsigned int     attr_index;
    sai_attribute_t *p_attr;
    unsigned int     idx;
    sai_object_id_t  grp_id;
    sai_object_id_t  nh_id;
    sai_status_t     rc;

    printf ("Converting SAI Next Hop Group member to NH Id...\n");

    for (attr_index = 0; attr_index < attr_count; attr_index++) {

        p_attr = &p_attr_list [attr_index];

        if (p_attr->id == SAI_NEXT_HOP_GROUP_ATTR_NEXT_HOP_MEMBER_LIST) {

            for (idx = 0; idx < p_attr->value.objlist.count; idx++) {
                rc = saiL3Test::sai_test_get_next_hop_group_info (
                                               p_attr->value.objlist.list[idx],
                                               &grp_id, &nh_id);

                if (rc != SAI_STATUS_SUCCESS) {
                    return rc;
                }

                if (in_nh_group_id != grp_id) {
                    return SAI_STATUS_INVALID_OBJECT_ID;
                }

                p_attr->value.objlist.list[idx] = nh_id;
            }
        }
    }

    return SAI_STATUS_SUCCESS;
}

static void sai_test_nh_group_attr_list_print (unsigned int attr_count,
                                               sai_attribute_t *p_attr_list)
{
    unsigned int     attr_index;
    sai_attribute_t *p_attr;
    unsigned int     idx;

    printf ("Printing SAI Next Hop Group attribute list..\n");
    for (attr_index = 0; attr_index < attr_count; attr_index++)
    {
        p_attr = &p_attr_list [attr_index];

        switch (p_attr->id) {

            case SAI_NEXT_HOP_GROUP_ATTR_NEXT_HOP_COUNT:
                printf ("Index: %d, Next Hop Group NH Count: %d.\n",
                        attr_index, p_attr->value.u32);
                break;

            case SAI_NEXT_HOP_GROUP_ATTR_NEXT_HOP_MEMBER_LIST:
                printf ("Index: %d, Next Hop Group NH List count: %d, "
                        "NH List NH Ids: \n", attr_index,
                        p_attr->value.objlist.count);

                for (idx = 0; idx < p_attr->value.objlist.count; idx++)
                {
                    printf ("0x%" PRIx64 " ", p_attr->value.objlist.list[idx]);
                }
                printf ("\n");
                break;

            case SAI_NEXT_HOP_GROUP_ATTR_TYPE:
                printf ("Index: %d, Next Hop Group Type: %d.\n", attr_index,
                        p_attr->value.s32);

                break;

            default:
                printf ("Index: %d, Next Hop unknown Attr Id: %d, Value: %ld.\n",
                        attr_index, p_attr->id, p_attr->value.u64);
                break;
        }
    }
}

/*
 * nh_group_id - [in] Next Hop Group ID.
 * p_attr_list - [in, out] pointer to attribute list.
 * attr_count  - [in] number of attributes to get.
 *                    For each attribute, SAI attribute id is passed.
 *
 * For attr_count = 2,
 * sai_test_nh_group_attr_get (nh_group_id, p_attr_list, 2, id_0, id_1)
 */
sai_status_t saiL3Test::sai_test_nh_group_attr_get (sai_object_id_t nh_group_id,
                                                    sai_attribute_t *p_attr_list,
                                                    unsigned int attr_count, ...)
{
    sai_status_t         sai_rc;
    va_list              varg_list;
    unsigned int         index;

    va_start (varg_list, attr_count);

    for (index = 0; index < attr_count; index++) {

        p_attr_list [index].id = va_arg (varg_list, unsigned int);
    }

    va_end (varg_list);

    sai_rc = p_sai_nh_grp_api_tbl->get_next_hop_group_attribute (nh_group_id,
                                                                 attr_count,
                                                                 p_attr_list);

    if (sai_rc != SAI_STATUS_SUCCESS) {

        printf ("SAI Next Hop Group Get attribute failed with error: %d.\n",
                sai_rc);
    } else {

        printf ("SAI Next Hop Group Get attribute success.\n");

        sai_rc = sai_test_convert_nh_grp_member_to_nh_id (nh_group_id,
                                                          attr_count,
                                                          p_attr_list);

        sai_test_nh_group_attr_list_print (attr_count, p_attr_list);
    }

    return sai_rc;
}

sai_status_t saiL3Test::sai_test_add_nh_to_group (
                                              sai_object_id_t group_id,
                                              unsigned int nh_count,
                                              sai_object_id_t *p_nh_list)
{
    sai_attribute_t attr_list [SAI_TEST_MAX_GROUP_NEXT_HOP_MEMBER_ATTR];
    sai_object_id_t member_id [nh_count];
    bool            map_created [nh_count];
    bool            member_created [nh_count];
    uint32_t        attr_index = 0;
    sai_status_t    sai_rc = SAI_STATUS_SUCCESS;
    uint32_t        index;
    uint32_t        tmp_index;

    memset (member_id, 0, sizeof (member_id));

    attr_list[attr_index].id = SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_GROUP_ID;
    attr_list[attr_index].value.oid = group_id;
    attr_index++;

    attr_list[attr_index].id = SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_ID;

    for (index = 0; index < nh_count; index++) {
        map_created [index] = false;
        member_created [index] = false;
        attr_list[attr_index].value.oid = p_nh_list [index];

        sai_rc = p_sai_nh_grp_api_tbl->
            create_next_hop_group_member(&member_id [index],
                    switch_id,
                    attr_index + 1, attr_list);

        if (sai_rc != SAI_STATUS_SUCCESS) {
            break;
        }

        member_created [index] = true;

        printf ("NH Member: 0x%" PRIx64 ", added successfully to the "
                "Group: 0x%" PRIx64 ".\n", member_id [index], group_id);
        sai_rc = sai_test_next_hop_group_map_add (group_id, p_nh_list [index],
                                                  member_id [index]);

        if (sai_rc != SAI_STATUS_SUCCESS) {
            break;
        }

        map_created [index] = true;
    }

    if (sai_rc == SAI_STATUS_SUCCESS) {
        printf ("SAI Add Next Hop to Group success for "
                "Group Id: 0x%" PRIx64 ".\n", group_id);

        return SAI_STATUS_SUCCESS;
    }

    printf ("SAI Add Next Hop to Group failed with error: %d.\n", sai_rc);

    for (tmp_index = 0; tmp_index <= index; tmp_index++) {
        if (member_created [tmp_index] == true) {
            p_sai_nh_grp_api_tbl->
                remove_next_hop_group_member (member_id [tmp_index]);

            if (map_created [tmp_index] == true) {
                sai_test_next_hop_group_map_del(group_id, p_nh_list [tmp_index],
                                                member_id [tmp_index]);
            }
        }
    }

    return sai_rc;
}

sai_status_t saiL3Test::sai_test_add_nh_to_group (
                                              sai_object_id_t group_id,
                                              unsigned int nh_count,
                                              sai_object_id_t *p_nh_list,
                                              sai_object_id_t *p_member_list)
{
    sai_attribute_t attr_list [SAI_TEST_MAX_GROUP_NEXT_HOP_MEMBER_ATTR];
    uint32_t        attr_index = 0;
    sai_status_t    sai_rc = SAI_STATUS_SUCCESS;
    uint32_t        index;
    uint32_t        tmp_index;

    attr_list[attr_index].id = SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_GROUP_ID;
    attr_list[attr_index].value.oid = group_id;
    attr_index++;

    attr_list[attr_index].id = SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_ID;

    for (index = 0; index < nh_count; index++) {
        attr_list[attr_index].value.oid = p_nh_list [index];

        sai_rc = p_sai_nh_grp_api_tbl->
            create_next_hop_group_member(&p_member_list [index],
                    switch_id,
                    attr_index + 1, attr_list);

        if (sai_rc != SAI_STATUS_SUCCESS) {
            break;
        }

        printf ("NH Member: 0x%" PRIx64 ", added successfully to the "
                "Group: 0x%" PRIx64 ".\n", p_member_list [index], group_id);
    }

    if (sai_rc == SAI_STATUS_SUCCESS) {
        printf ("SAI Add Next Hop to Group success for "
                "Group Id: 0x%" PRIx64 ".\n", group_id);

        return SAI_STATUS_SUCCESS;
    }

    printf ("SAI Add Next Hop to Group failed with error: %d.\n", sai_rc);

    for (tmp_index = 0; tmp_index < index; tmp_index++) {
        p_sai_nh_grp_api_tbl->
            remove_next_hop_group_member (p_member_list [tmp_index]);
    }

    return sai_rc;
}

sai_status_t saiL3Test::sai_test_remove_nh_from_group (
                                              sai_object_id_t group_id,
                                              unsigned int nh_count,
                                              sai_object_id_t *p_nh_list)
{
    sai_status_t    sai_rc;
    sai_object_id_t member_id;
    uint32_t        index;

    for (index = 0; index < nh_count; index++) {

        sai_rc = sai_test_get_next_hop_member_id (group_id, p_nh_list [index],
                                                  &member_id);

        if (sai_rc != SAI_STATUS_SUCCESS) {
            return sai_rc;
        }

        sai_rc = p_sai_nh_grp_api_tbl->
            remove_next_hop_group_member (member_id);

        if (sai_rc != SAI_STATUS_SUCCESS) {
            printf ("SAI Remove NH member from Group failed. Group Id: "
                    "0x%" PRIx64 ", NH Id: 0x%" PRIx64 ", Member Id: "
                    "0x%" PRIx64 ". Error: %d.\n", group_id, p_nh_list[index],
                    member_id, sai_rc);
        }
        else {
            printf ("SAI Remove NH member from Group success. Group Id: "
                    "0x%" PRIx64 ", NH Id: 0x%" PRIx64 ", Member Id: "
                    "0x%" PRIx64 ".\n", group_id, p_nh_list[index],
                    member_id);
        }

        sai_test_next_hop_group_map_del (group_id, p_nh_list[index], member_id);
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t saiL3Test::sai_test_next_hop_group_map_add (
                                            sai_object_id_t grp_id,
                                            sai_object_id_t nh_id,
                                            sai_object_id_t member_id)
{
    sai_l3_test_nh_grp_info_t grp_info;

    memset (&grp_info, 0, sizeof (grp_info));
    grp_info.nh_grp_id = grp_id;
    grp_info.nh_id     = nh_id;

    printf ("Map ADD. Group Id: 0x%" PRIx64 ", NH Id: 0x%" PRIx64 ", "
            "Member Id: 0x%" PRIx64 ".\n", grp_id, nh_id, member_id);
    try {
        _nh_grp_info_2_member.insert (std::make_pair (grp_info, member_id));
        _nh_member_2_grp_info.insert (std::make_pair (member_id, grp_info));
    }
    catch (...) {
        return SAI_STATUS_FAILURE;
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t saiL3Test::sai_test_next_hop_group_map_del (
                                            sai_object_id_t grp_id,
                                            sai_object_id_t nh_id,
                                            sai_object_id_t member_id)
{
    sai_l3_test_nh_grp_info_t grp_info;

    memset (&grp_info, 0, sizeof (grp_info));
    grp_info.nh_grp_id = grp_id;
    grp_info.nh_id     = nh_id;

    printf ("Map DELETE. Group Id: 0x%" PRIx64 ", NH Id: 0x%" PRIx64 ", "
            "Member Id: 0x%" PRIx64 ".\n", grp_id, nh_id, member_id);
    try {
        _nh_grp_info_2_member.erase (grp_info);
        _nh_member_2_grp_info.erase (member_id);
    }
    catch (...) {
        return SAI_STATUS_FAILURE;
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t saiL3Test::sai_test_get_next_hop_group_info (
                                             sai_object_id_t  member_id,
                                             sai_object_id_t *out_grp_id,
                                             sai_object_id_t *out_nh_id)
{
    try {
        auto it = _nh_member_2_grp_info.find (member_id);

        if (it == _nh_member_2_grp_info.end ()) {
            return SAI_STATUS_FAILURE;
        }

        sai_l3_test_nh_grp_info_t& grp_info = it->second;

        *out_grp_id = grp_info.nh_grp_id;
        *out_nh_id  = grp_info.nh_id;
    }
    catch (...) {
        return SAI_STATUS_FAILURE;
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t saiL3Test::sai_test_get_next_hop_member_id (
                                            sai_object_id_t  grp_id,
                                            sai_object_id_t  nh_id,
                                            sai_object_id_t *out_member_id)
{
    sai_l3_test_nh_grp_info_t grp_info;

    memset (&grp_info, 0, sizeof (grp_info));

    grp_info.nh_grp_id = grp_id;
    grp_info.nh_id     = nh_id;

    try {
        auto it = _nh_grp_info_2_member.find (grp_info);

        if (it == _nh_grp_info_2_member.end ()) {
            return SAI_STATUS_FAILURE;
        }

        *out_member_id = it->second;
    }
    catch (...) {
        return SAI_STATUS_FAILURE;
    }

    return SAI_STATUS_SUCCESS;
}

void saiL3Test::sai_test_print_next_hop_info_map (bool print_all_groups,
                                                  sai_object_id_t grp_id)
{
    uint32_t     index = 0;

    try {
        for (auto& map_kv: _nh_grp_info_2_member) {
            const sai_l3_test_nh_grp_info_t& grp_info = map_kv.first;

            if ((print_all_groups) || (grp_info.nh_grp_id == grp_id)) {
                index++;
                printf ("### [%03d] Group Id: 0x%" PRIx64 ", NH Id: "
                        "0x%" PRIx64 ", Member Id: 0x%" PRIx64 ".\n", index,
                        grp_info.nh_grp_id, grp_info.nh_id, map_kv.second);
            }
        }
    }
    catch (...) {
        return;
    }

    return;
}

sai_status_t saiL3Test::sai_test_get_nh_count (sai_object_id_t  group_id,
                                               uint32_t        *out_nh_count)
{
    sai_status_t rc = SAI_STATUS_SUCCESS;
    uint32_t     index = 0;

    try {
        for (auto& map_kv: _nh_grp_info_2_member) {
            const sai_l3_test_nh_grp_info_t& grp_info = map_kv.first;

            if (grp_info.nh_grp_id == group_id) {
                index++;
            }
        }
    }
    catch (...) {
        rc = SAI_STATUS_FAILURE;
    }

    *out_nh_count = index;
    return rc;
}

sai_status_t saiL3Test::sai_test_get_nh_list (sai_object_id_t  group_id,
                                              uint32_t        *in_out_nh_count,
                                              sai_object_id_t *out_nh_list)
{
    sai_status_t rc = SAI_STATUS_SUCCESS;
    uint32_t     index = 0;

    try {
        for (auto& map_kv: _nh_grp_info_2_member) {
            const sai_l3_test_nh_grp_info_t& grp_info = map_kv.first;

            if (grp_info.nh_grp_id == group_id) {
                if (index >= (*in_out_nh_count)) {
                    printf("sai_test_get_nh_list(). Buffer Overflow. "
                           "group_id: 0x%" PRIx64 ", index: %d, in_count: %d\n",
                           group_id, index, *in_out_nh_count);
                    rc = SAI_STATUS_FAILURE;
                    break;
                }

                out_nh_list [index] = grp_info.nh_id;
                index++;
            }
        }
    }
    catch (...) {
        rc = SAI_STATUS_FAILURE;
    }

    *in_out_nh_count = index;
    return rc;
}


/*
 * Helper function to create an FDB entry.
 */
sai_status_t saiL3Test::sai_test_neighbor_fdb_entry_create (
                                                       const char *p_mac_str,
                                                       sai_object_id_t vlan_obj_id,
                                                       sai_object_id_t bridge_port_id)
{
    sai_fdb_entry_t    fdb_entry;
    sai_status_t       status;
    const unsigned int default_fdb_attr_count = 3;
    sai_attribute_t    attr_list [default_fdb_attr_count];

    memset (&fdb_entry, 0, sizeof (sai_fdb_entry_t));

    sai_test_router_mac_str_to_bytes_get (p_mac_str, fdb_entry.mac_address);

    fdb_entry.bv_id = vlan_obj_id;

    attr_list[0].id = SAI_FDB_ENTRY_ATTR_TYPE;
    attr_list[0].value.s32 = SAI_FDB_ENTRY_TYPE_DYNAMIC;

    attr_list[1].id = SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID;
    attr_list[1].value.oid = bridge_port_id;

    attr_list[2].id = SAI_FDB_ENTRY_ATTR_PACKET_ACTION;
    attr_list[2].value.s32 = SAI_PACKET_ACTION_FORWARD;

    status = p_sai_fdb_api_table->create_fdb_entry (
                                        (const sai_fdb_entry_t*)&fdb_entry,
                                        default_fdb_attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS) {

        printf ("SAI FDB entry creation failed with error: %d.\n", status);

    } else {

        printf ("SAI FDB entry creation success.\n");
    }

    return status;
}

/*
 * Helper function to remove an FDB entry.
 */
sai_status_t saiL3Test::sai_test_neighbor_fdb_entry_remove (
                                                       const char *p_mac_str,
                                                       sai_object_id_t vlan_obj_id)
{
    sai_fdb_entry_t    fdb_entry;
    sai_status_t       status;

    memset (&fdb_entry, 0, sizeof (sai_fdb_entry_t));

    sai_test_router_mac_str_to_bytes_get (p_mac_str, fdb_entry.mac_address);

    fdb_entry.bv_id = vlan_obj_id;

    status = p_sai_fdb_api_table->remove_fdb_entry (
                                        (const sai_fdb_entry_t*)&fdb_entry);

    if (status != SAI_STATUS_SUCCESS) {

        printf ("SAI FDB entry remove failed with error: %d.\n", status);

    } else {

        printf ("SAI FDB entry remove success.\n");
    }

    return status;
}

sai_status_t saiL3Test::sai_test_neighbor_fdb_entry_port_set (
                                                       const char *p_mac_str,
                                                       sai_object_id_t vlan_obj_id,
                                                       sai_object_id_t bridge_port_id)
{
    sai_fdb_entry_t    fdb_entry;
    sai_status_t       status;
    sai_attribute_t    attr;

    memset (&fdb_entry, 0, sizeof (sai_fdb_entry_t));

    sai_test_router_mac_str_to_bytes_get (p_mac_str, fdb_entry.mac_address);

    fdb_entry.bv_id = vlan_obj_id;

    attr.id = SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID;
    attr.value.oid = bridge_port_id;

    status = p_sai_fdb_api_table->set_fdb_entry_attribute (
                                        (const sai_fdb_entry_t*)&fdb_entry, &attr);

    if (status != SAI_STATUS_SUCCESS) {

        printf ("SAI FDB entry set attribute failed with error: %d.\n", status);

    } else {

        printf ("SAI FDB entry set attribute success.\n");
    }

    return status;
}

sai_status_t saiL3Test ::sai_test_remove_port_from_vlan(
        sai_vlan_api_t *sai_vlan_api_tbl,
        sai_object_id_t vlan_obj_id, sai_object_id_t port_id)
{
    sai_status_t    sai_rc = SAI_STATUS_SUCCESS;
    sai_attribute_t attr;
    int port_count=0;
    int it=0;

    attr.id = SAI_VLAN_ATTR_MEMBER_LIST;
    attr.value.objlist.count = 0;
    sai_object_id_t bridge_port_id = 0;

    /* Getting the number of ports using count as 0 */
    sai_rc = sai_vlan_api_tbl->get_vlan_attribute(vlan_obj_id,1,&attr);
    if(SAI_STATUS_BUFFER_OVERFLOW != sai_rc) {
        printf("Unable to get the VLAN member list count for"
                " VLAN obj id:%lu, rc:%d\r\n",vlan_obj_id,sai_rc);
        return SAI_STATUS_FAILURE;
    }
    port_count = attr.value.objlist.count;
    printf("VLAN member list count for VLAN obj id:%lu is %d\r\n",
            vlan_obj_id,attr.value.objlist.count);

    {
        sai_object_id_t list[port_count];

        /* Getting the list of ports in default vlan */
        attr.value.objlist.count = port_count;
        attr.value.objlist.list = list;
        sai_rc = sai_vlan_api_tbl->get_vlan_attribute(vlan_obj_id,1,&attr);
        if(SAI_STATUS_SUCCESS != sai_rc) {
            printf("Unable to get the VLAN member list for"
                    " VLAN obj id:%lu, rc:%d\r\n",vlan_obj_id,sai_rc);
            return SAI_STATUS_FAILURE;
        }

        attr.id = SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID;
        attr.value.oid = 0;
        /* Finding the member id of the port id */
        for(it=0; it<port_count; it++) {
            sai_rc = sai_vlan_api_tbl->get_vlan_member_attribute(list[it],1,&attr);
            if(SAI_STATUS_SUCCESS != sai_rc) {
                printf("Unable to get the VLAN member port id for "
                        "member:%lu in VLAN obj:%lu, rc:%d\r\n",
                        list[it],vlan_obj_id,sai_rc);
            }
            bridge_port_id = attr.value.oid;
            attr.id = SAI_BRIDGE_PORT_ATTR_PORT_ID;

            sai_rc  = p_sai_bridge_api_tbl->get_bridge_port_attribute(bridge_port_id,
                                                                      1, &attr);
            if(SAI_STATUS_SUCCESS != sai_rc) {
                printf("Unable to get the port id for "
                        "bridge port:%lu  rc:%d\r\n",
                        bridge_port_id,sai_rc);
            }

            if(attr.value.oid == port_id) {
                sai_rc = sai_vlan_api_tbl->remove_vlan_member(list[it]);
                if(SAI_STATUS_SUCCESS != sai_rc) {
                    printf("Failed member remove for VLAN member:%lu "
                            "in VLAN obj:%lu, rc:%d\r\n",
                            list[it],vlan_obj_id,sai_rc);
                    return SAI_STATUS_FAILURE;
                }
                return SAI_STATUS_SUCCESS;
            }
        }
    }

    printf("Unable to find port:%lu as member in VLAN obj:%lu",
            port_id,vlan_obj_id);

    return SAI_STATUS_FAILURE;
}

sai_status_t saiL3Test ::sai_test_add_port_to_vlan(
        sai_object_id_t switch_id,
        sai_vlan_api_t *sai_vlan_api_tbl,
        sai_object_id_t vlan_obj_id, sai_object_id_t port_id)
{
    sai_status_t    sai_rc = SAI_STATUS_SUCCESS;
    sai_object_id_t vlan_member_id;
    sai_attribute_t attr[3];
    uint32_t attr_count = 0;
    sai_object_id_t bridge_port_id = SAI_NULL_OBJECT_ID;
    sai_bridge_api_t *p_sai_bridge_api_tbl;
    sai_switch_api_t *p_sai_switch_api_tbl;

    sai_api_query(SAI_API_BRIDGE, (static_cast<void**>
                                 (static_cast<void*>(&p_sai_bridge_api_tbl))));
    sai_api_query(SAI_API_SWITCH, (static_cast<void**>
                                 (static_cast<void*>(&p_sai_switch_api_tbl))));
    sai_rc = sai_bridge_ut_get_bridge_port_from_port(p_sai_switch_api_tbl, p_sai_bridge_api_tbl,
                                                 switch_id, port_id, & bridge_port_id);

    if(sai_rc != SAI_STATUS_SUCCESS) {
        return sai_rc;
    }
    memset(&attr,0,sizeof(attr));
    attr[attr_count].id = SAI_VLAN_MEMBER_ATTR_VLAN_ID;
    attr[attr_count].value.oid = vlan_obj_id;
    attr_count++;
    attr[attr_count].id = SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID;
    attr[attr_count].value.oid = bridge_port_id;
    attr_count++;
    attr[attr_count].id = SAI_VLAN_MEMBER_ATTR_VLAN_TAGGING_MODE;
    attr[attr_count].value.u32 = SAI_VLAN_TAGGING_MODE_UNTAGGED;
    attr_count++;
    sai_rc = sai_vlan_api_tbl->create_vlan_member(&vlan_member_id, 0,
            attr_count, attr);

    if(sai_rc != SAI_STATUS_SUCCESS) {
        return SAI_STATUS_FAILURE;
    }
    return SAI_STATUS_SUCCESS;
}

sai_object_id_t saiL3Test ::sai_l3_default_vlan_obj_id_get (void)
{
    return default_vlan_id;
}

void saiL3Test ::sai_l3_default_vlan_obj_id_set(sai_object_id_t vlan_obj_id)
{
    default_vlan_id = vlan_obj_id;
}
