with Primitive_Base;
package Primitive_Child is
    type T is new Primitive_Base.T;
    overriding function "=" (X, Y : T) return Boolean;
    overriding function Read (X : T) return Integer renames Read;
    type Leaf is new T;
end Primitive_Child;
