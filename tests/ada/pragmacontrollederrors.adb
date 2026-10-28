procedure PragmaControlledErrors is
    type Pointer is access Integer;
    pragma Controlled (Pointer);
    pragma Controlled (Pointer);
    subtype View is Pointer;
    pragma Controlled (View);
    type Derived is new Pointer;
    pragma Controlled (Derived);
    Object : Pointer;
    pragma Controlled (Object);
    pragma Controlled (Integer);
begin
    null;
end PragmaControlledErrors;
