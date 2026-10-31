procedure AnonymousAccessErrors is
    type Link is access all Integer;
    Global : aliased Integer := 1;
    Saved : access Integer := Global'Access;
    Named : Link;
    procedure Constant_View (P : access constant Integer) is
    begin
        P.all := 2;
    end Constant_View;
    function Escape return access Integer is
        Local : aliased Integer := 3;
    begin
        return Local'Access;
    end Escape;
    procedure Callback_Escape (Action : access procedure) is
        Stored : access procedure := Action;
    begin
        null;
    end Callback_Escape;
    type Bad_Default (Ref : access Integer := null) is null record;
    type Container is record
        Ref : access Integer;
    end record;
    Object : Container;
    procedure Wrong (P : access Integer) is null;
    Read_Only : access constant Integer := Global'Access;
begin
    Named := Saved;
    Wrong (Read_Only);
    declare
        Local : aliased Integer;
        type Local_Link is access all Integer;
        Value : Local_Link := Local'Access;
    begin
        Saved := Value;
        Object.Ref := Value;
    end;
end AnonymousAccessErrors;
