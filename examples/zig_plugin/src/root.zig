const std = @import("std");
const testing = std.testing;

const labraven_modes = @cImport(
    {
        @cInclude("LabActivity.h");
    }
);

export fn make_activity() labraven_modes.LabActivity
{
    const result = labraven_modes.LabActivity {
        .Activate = MyActivity.Activate,
        .Deactivate = MyActivity.Deactivate,
        .Update = MyActivity.Update,
        .Render = MyActivity.Render,
        .RunUI = MyActivity.RunUI,
        .name = ma.name,
    };

    return result;
}

var ma = MyActivity{
    .name = "global",
};

const MyActivity = struct {
    export fn Activate(
        self: ?*anyopaque,
    ) void 
    {
        const actual : *MyActivity = @alignCast(@ptrCast(self));

        actual.active = true;
    }
    export fn Deactivate(
        self: ?*anyopaque,
    ) void 
    {
        const actual : *MyActivity = @alignCast(@ptrCast(self));

        actual.active = false;
    }

    export fn Update(
        self: ?*anyopaque,
    ) void 
    {
        const actual : *MyActivity = @alignCast(@ptrCast(self));

        actual.update_count += 1;

        std.debug.print("Updated! {d}\n", .{ actual.update_count });
    }

    export fn Render(
        self: ?*anyopaque,
        _: [*c]const labraven_modes.LabViewInteraction,
    ) void 
    {
        const actual : *MyActivity = @alignCast(@ptrCast(self));

        actual.render_count += 1;

        std.debug.print("Rendered! {d}\n", .{ actual.render_count });
    }

    export fn RunUI(
        _: ?*anyopaque,
        _: [*c]const labraven_modes.LabViewInteraction,
    ) void 
    {
        std.debug.print("runui!\n", .{});
    }

    export fn Menu(
        _: ?*anyopaque,
    ) void 
    {
        std.debug.print("menu!\n", .{});
    }

    export fn ToolBar(
        _: ?*anyopaque,
    ) void 
    {
        std.debug.print("ToolBar!\n", .{});
    }

    export fn ViewportHoverBid(
        _: ?*anyopaque,
        _: *const labraven_modes.LabViewInteraction
    ) c_int   
    {
        std.debug.print("ToolBar!\n", .{});

        return 0;
    }

    export fn ViewportHovering(
        _: ?*anyopaque,
        _: *const labraven_modes.LabViewInteraction
    ) void  
    {
        std.debug.print("ViewportHovering!\n", .{});
    }

    export fn ViewportDragBid(
        _: ?*anyopaque,
        _: *const labraven_modes.LabViewInteraction
    ) c_int   
    {
        std.debug.print("ViewportDragBid!\n", .{});
        return 0;
    }
    export fn ViewportDragging(
        _: ?*anyopaque,
        _: [*c]const labraven_modes.LabViewInteraction
    ) void  
    {
        std.debug.print("ViewportDragging!\n", .{});
    }

    name: [*:0]const u8,
    active: bool = false,

    update_count : usize=0,
    render_count : usize=0,
};


export fn add(a: i32, b: i32) i32 {
    return a + b;
}

test "basic add functionality" {
    try testing.expect(add(3, 7) == 10);
}
