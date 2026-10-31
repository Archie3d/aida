procedure NullGenericCompletionErrors is
    generic
    procedure Template;
    procedure Template is null;
begin
    null;
end NullGenericCompletionErrors;
