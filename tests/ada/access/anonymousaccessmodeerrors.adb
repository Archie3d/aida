procedure AnonymousAccessModeErrors is
    procedure Bad (P : in access Integer) is null;
begin
    null;
end AnonymousAccessModeErrors;
