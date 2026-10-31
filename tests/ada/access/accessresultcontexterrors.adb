with Access_Result_Base;
procedure AccessResultContextErrors is
    procedure Check (Self : access Access_Result_Base.Root'Class) is
        Bad : access Access_Result_Base.Root := Access_Result_Base.Identity (Self);
    begin
        null;
    end Check;
begin
    null;
end AccessResultContextErrors;
