#include "models.h"

Item::Item(int id) {
    this->id = id;
    this->utility = 0;
}

Sequence::Sequence() {
    this->size = 0;
    this->utility = 0;
}

ItemInstance::ItemInstance(float utility, float rem, unsigned int position) {
    this->utility = utility;
    this->position = position;
    this->rem = rem;
}

Pattern::Pattern() {
    this->name = "";
    this->lastItem = -1;
    this->parentLastItem = -1;
    this->do_ext = true;
    this->do_s_ext = true;
    this->size = 0;
    this->isMaximal = false;
    this->isSExt = true;
    this->isParentSExt = true;
    this->umin = 0;
    this->RBU = 0;
    this->SE = 0;
    this->SLIP = 0;
}
