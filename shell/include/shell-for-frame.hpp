/* Fixed for frame and validation, shared by guest execution and native checks. */
struct ShellForFrame {
    unsigned int active, count, index, root, entry, body, next, join;
    char name[32];
    char values[1152];
};
static ShellForFrame gForFrame;
static void copyShellForFrame(ShellForFrame* to, const ShellForFrame* from)
{
    to->active = from->active; to->count = from->count; to->index = from->index;
    to->root = from->root; to->entry = from->entry; to->body = from->body;
    to->next = from->next; to->join = from->join;
    for (unsigned int i = 0; i < 32; ++i) to->name[i] = from->name[i];
    for (unsigned int i = 0; i < 1152; ++i) to->values[i] = from->values[i];
}
static int validForCaller(const ShellForFrame* frame, unsigned int pc)
{
    if (!frame->active) return 1;
    return gForCount == 1 && frame->active == 1 && frame->count && frame->count <= 12
        && frame->index < frame->count && frame->root == gForPlan.root
        && frame->entry == gForPlan.entry && frame->body == gForPlan.body
        && frame->next == gForPlan.next && frame->join == gForPlan.join
        && pc >= frame->body && pc <= frame->next;
}
