const std = @import("std");

pub fn main() !void {
    // Prints to stderr (it's a shortcut based on `std.io.getStdErr()`)
    std.debug.print("All your {s} are belong to us.\n", .{"codebase"});

    // stdout is for the actual output of your application, for example if you
    // are implementing gzip, then only the compressed bytes should be sent to
    // stdout, not any debugging messages.
    const stdout_file = std.io.getStdOut().writer();
    var bw = std.io.bufferedWriter(stdout_file);
    const stdout = bw.writer();

    try stdout.print("Run `zig build test` to run the tests.\n", .{});

    try bw.flush(); // don't forget to flush!
}


test "simple test" {
    var list = std.ArrayList(i32).init(std.testing.allocator);
    defer list.deinit(); // try commenting this out and see if zig detects the memory leak!
    try list.append(42);
    try std.testing.expectEqual(@as(i32, 42), list.pop());
}


// Define your plugin name function
export fn PluginName() [*:0] const u8 {
    return "labraventest!";
}

// cr.h enums and structs
const cr_op = enum {
    CR_LOAD,
    CR_STEP,
    CR_UNLOAD,
    CR_CLOSE,
};

const cr_result = enum {
    CR_NONE,
    CR_OTHER,
};

pub const cr_plugin = extern struct {};

// The main entry point for the plugin as mandated by cr.h
export fn cr_main(_: *cr_plugin, operation: i32) i32 { //cr_result {
    switch (operation) {
        0, //cr_op.CR_LOAD,
        1, //cr_op.CR_UNLOAD,
        2, //cr_op.CR_CLOSE,
        3 => return 0, //cr_op.CR_STEP => return cr_result.CR_NONE,
        else => return 0, // cr_result.CR_OTHER,
    }
}

