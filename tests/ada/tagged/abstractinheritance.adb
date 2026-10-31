package Abstract_Inheritance_Base is
    type Root is tagged record
        Value : Integer := 8;
    end record;
    function Make return Root;
    type Equal_Root is abstract tagged record
        Value : Integer := 0;
    end record;
    function "=" (Left, Right : Equal_Root) return Boolean is abstract;
end Abstract_Inheritance_Base;
package body Abstract_Inheritance_Base is
    function Make return Root is
    begin
        return (Value => 8);
    end Make;
end Abstract_Inheritance_Base;

with Abstract_Inheritance_Base;
package Abstract_Inheritance_Children is
    type Middle is abstract new Abstract_Inheritance_Base.Root with record
        Extra : Integer := 5;
    end record;
    type Leaf is new Middle with null record;
    overriding function Make return Leaf;
    type Equal_Leaf is new Abstract_Inheritance_Base.Equal_Root with null record;
    overriding function "=" (Left, Right : Equal_Leaf) return Boolean;
    type Another is abstract new Abstract_Inheritance_Base.Equal_Root with null record;
private
    -- An abstract override may be private when the inherited obligation is visible.
    overriding function "=" (Left, Right : Another) return Boolean is abstract;
end Abstract_Inheritance_Children;
package body Abstract_Inheritance_Children is
    overriding function Make return Leaf is
    begin
        return (Value => 13, Extra => 7);
    end Make;
    overriding function "=" (Left, Right : Equal_Leaf) return Boolean is
    begin
        return Left.Value = Right.Value;
    end "=";
end Abstract_Inheritance_Children;

with Ada.Text_IO;
with Abstract_Inheritance_Base;
with Abstract_Inheritance_Children;
procedure AbstractInheritance is
    procedure Run is
        use Abstract_Inheritance_Children;
        use type Abstract_Inheritance_Base.Equal_Root;
        Item : Leaf := Make;
        Left : Equal_Leaf := (Value => 4);
        Right : Equal_Leaf := (Value => 5);
        Wide_Left : Abstract_Inheritance_Base.Equal_Root'Class := Left;
        Wide_Right : Abstract_Inheritance_Base.Equal_Root'Class := Right;
    begin
        if Item.Value /= 13 or else Item.Extra /= 7 then
            raise Program_Error;
        end if;
        if Wide_Left = Wide_Right or else not (Wide_Left /= Wide_Right) then
            raise Program_Error;
        end if;
        Wide_Right := Wide_Left;
        if not (Wide_Left = Wide_Right) then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("abstractinheritance worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("abstract inheritance passed");
end AbstractInheritance;
