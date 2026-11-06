generic
    Seed : Integer;
package Late_Child_Parent is
    Value : Integer := Seed;
end Late_Child_Parent;

generic
package Late_Child_Parent.Child is
    procedure Add;
end Late_Child_Parent.Child;

with Ada.Text_IO;
with Late_Child_Parent.Child;
procedure GenericChildLate is
    package Parent is new Late_Child_Parent (3);
    package Child is new Parent.Child;
begin
    Child.Add;
    if Parent.Value /= 4 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("late generic child body passed");
end GenericChildLate;

package body Late_Child_Parent.Child is
    procedure Add is
    begin
        Value := Value + 1;
    end Add;
end Late_Child_Parent.Child;
