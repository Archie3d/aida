with Callback_Inherited_Parent;
package Callback_Inherited_Child is
    type Derived is new Callback_Inherited_Parent.Root with null record;
end Callback_Inherited_Child;
