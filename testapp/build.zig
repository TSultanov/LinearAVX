const std = @import("std");

pub fn build(b: *std.Build) void {
    const features = std.Target.x86.Feature;
    var enabled_features = std.Target.Cpu.Feature.Set.empty;
    enabled_features.addFeature(@intFromEnum(features.avx));

    const target = b.standardTargetOptions(.{ .default_target = .{
        .os_tag = .windows,
        .cpu_arch = .x86_64,
        .cpu_features_add = enabled_features,
        .abi = .gnu,
    } });

    // Standard optimization options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall. Here we do not
    // set a preferred release mode, allowing the user to decide how to optimize.
    const optimize = b.standardOptimizeOption(.{ .preferred_optimize_mode = .ReleaseSmall });

    const exe = b.addExecutable(.{
        .name = "testapp",
        // In this case the main source file is merely a path, however, in more
        // complicated build scripts, this could be a generated file.
        .root_source_file = .{ .path = "main.c" },
        .target = target,
        .optimize = optimize,
    });
    exe.linkLibC();

    b.installArtifact(exe);
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
