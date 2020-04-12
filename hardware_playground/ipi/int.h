#pragma once

void setup_idt();
int setup_inthandler(int index, void (*handler)());
