with Ada.Text_IO; use Ada.Text_IO;
with Derived_Model;
procedure DerivedLibrary is
    type Local is new Derived_Model.Number;
    type Sibling is new Derived_Model.Number;
    type Shade is new Derived_Model.Middle;
    N : Derived_Model.Number := Derived_Model.Make (12);
    X : Local := Local (N);
    Y : Sibling := Sibling (X);
    C : Derived_Model.Color := Derived_Model.Green;
    S : Shade := Shade (C);
begin
    Put_Line (Integer'Image (Read (X)));
    Put_Line (Integer'Image (Read (Local'(Make (25)))));
    Put_Line (Integer'Image (Read (Y)));
    Put_Line (Derived_Model.Color'Image (Derived_Model.Color (S)));
end DerivedLibrary;
