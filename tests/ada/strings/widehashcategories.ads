with Ada.Wide_Characters;
with Ada.Wide_Wide_Characters;
with Ada.Strings.Wide_Fixed.Wide_Hash;
with Ada.Strings.Wide_Wide_Fixed.Wide_Wide_Hash;
with Ada.Containers;
package WideHashCategories is
    pragma Pure;
    function Hash (Key : Wide_String) return Ada.Containers.Hash_Type
        renames Ada.Strings.Wide_Fixed.Wide_Hash;
    function Hash (Key : Wide_Wide_String) return Ada.Containers.Hash_Type
        renames Ada.Strings.Wide_Wide_Fixed.Wide_Wide_Hash;
end WideHashCategories;
