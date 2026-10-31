with Ada.Text_IO; use Ada.Text_IO;
with Callback_Inherited_Child; use Callback_Inherited_Child;
procedure InheritedCallbackLibrary is
    type Reader is access function (Object : Derived) return Integer;
    type Setter is access procedure (Object : in out Derived; Value : Integer);
    type Constructor is access function (Value : Integer) return Derived;
    F : Reader := Read'Access;
    P : Setter := Set'Access;
    Make : Constructor := Create'Access;
    X : Derived := Make (41);
begin
    if F (X) /= 41 then raise Program_Error; end if;
    P (X, 42);
    if F (X) /= 42 or X'Tag /= Derived'Tag then raise Program_Error; end if;
    Put_Line ("library inherited callbacks passed");
end InheritedCallbackLibrary;
