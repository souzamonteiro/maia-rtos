/* Fixed shell-instance storage. Syntax/name checks belong to the generated
   word frontend; table operations receive already validated byte strings. */
static char gVariableNames[1024];
static char gVariableValues[3072];
static int shellVariableIndex(const char* name, unsigned int length)
{
    for (unsigned int slot = 0; slot < 32; ++slot) {
        const char* stored = gVariableNames + slot * 32;
        if (!stored[0]) continue;
        unsigned int i = 0;
        while (i < length && stored[i] && stored[i] == name[i]) ++i;
        if (i == length && !stored[i]) return static_cast<int>(slot);
    }
    return -1;
}
static const char* shellVariableValue(const char* name, unsigned int length)
{
    const int slot = shellVariableIndex(name, length);
    return slot < 0 ? "" : gVariableValues + static_cast<unsigned int>(slot) * 96;
}
static int setShellVariable(const char* name, const char* value)
{
    unsigned int length = 0; while (name[length]) ++length;
    int slot = shellVariableIndex(name, length);
    if (slot < 0) {
        for (unsigned int i = 0; i < 32; ++i)
            if (!gVariableNames[i * 32]) { slot = static_cast<int>(i); break; }
    }
    if (slot < 0) return 0;
    char* storedName = gVariableNames + static_cast<unsigned int>(slot) * 32;
    char* storedValue = gVariableValues + static_cast<unsigned int>(slot) * 96;
    for (unsigned int i = 0; i <= length; ++i) storedName[i] = name[i];
    unsigned int i = 0;
    do { storedValue[i] = value[i]; } while (value[i++]);
    return 1;
}
static void unsetShellVariable(const char* name)
{
    unsigned int length = 0; while (name[length]) ++length;
    const int slot = shellVariableIndex(name, length);
    if (slot >= 0) {
        gVariableNames[static_cast<unsigned int>(slot) * 32] = 0;
        gVariableValues[static_cast<unsigned int>(slot) * 96] = 0;
    }
}
