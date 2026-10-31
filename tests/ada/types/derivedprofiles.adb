with Ada.Text_IO; use Ada.Text_IO;
with Derived_Profile_Model;
procedure DerivedProfiles is
    package Derived is
        type Disk is new Derived_Profile_Model.Circle;
    end Derived;
    package Further is
        type Grandchild is new Derived.Disk;
    end Further;
    type Any_Item is new Derived_Profile_Model.Item;
    Flat : Any_Item (False) := (False, 12);
    Ball : Any_Item (True) := (True, 10);
begin
    -- The inherited result and unconstrained formal retain the parent's
    -- profile, even though Disk's first subtype fixes Round to True.
    Put_Line (Boolean'Image (Further.Inspect (Further.Make (False))));
    Put_Line (Integer'Image (Further.Make (False).Width));
    Put_Line (Integer'Image (Read (Ball)));
    begin
        declare
            Object : Derived.Disk := Derived.Make (False);
        begin
            Put_Line ("MISSED result constraint");
        end;
    exception
        when Constraint_Error => Put_Line ("result constraint");
    end;
    begin
        Put_Line (Integer'Image (Derived.Read (Derived.Make (False))));
    exception
        when Constraint_Error => Put_Line ("inherited formal constraint");
    end;
    begin
        Put_Line (Integer'Image (Read (Flat)));
    exception
        when Constraint_Error => Put_Line ("in constraint");
    end;
    begin
        Touch (Flat);
    exception
        when Constraint_Error => Put_Line ("in out constraint");
    end;
    begin
        Reset (Flat);
    exception
        when Constraint_Error => Put_Line ("out constraint");
    end;
    Put_Line (Integer'Image (Derived_Profile_Model.Calls));
    Put_Line (Integer'Image (Flat.Width));
end DerivedProfiles;
