Jekyll::Hooks.register :site, :post_write do |site|
  # Only run in production; skip in development to keep things fast and simple
  return unless Jekyll.env == 'production'

  assets_dir = File.join(site.dest, 'assets')
  return unless File.directory?(assets_dir)

  css_files = Dir.glob(File.join(assets_dir, '**', '*.css')).reject { |p| p.end_with?('.map') }
  js_files = Dir.glob(File.join(assets_dir, '**', '*.js'))

  # Build rename map: original_path => hashed_path
  rename_map = {}
  (css_files + js_files).each do |path|
    rename_map[path] = compute_hashed_path(path)
  end

  # Clean up old hashed variants before renaming.
  # Use triple-dash separator so the pattern basename---*.<ext> is unambiguous.
  rename_map.each do |orig_path, new_path|
    dir = File.dirname(orig_path)
    base = File.basename(orig_path, '.*')
    ext = File.extname(orig_path)

    Dir.glob(File.join(dir, "#{base}---*#{ext}")).each do |old_hashed|
      # Skip if this is already the correct (new) path
      next if old_hashed == new_path
      FileUtils.rm(old_hashed)
      puts "  [Asset Digest] Removed old variant: #{File.basename(old_hashed)}"
    end
  end

  # Rename files
  rename_map.each do |old_path, new_path|
    if old_path != new_path && File.exist?(old_path)
      FileUtils.mv(old_path, new_path)
      puts "  [Asset Digest] #{File.basename(old_path)} -> #{File.basename(new_path)}"
    end
  end

  # Rewrite HTML references
  Dir.glob(File.join(site.dest, '**', '*.html')).each do |html_path|
    html = File.read(html_path)
    modified = false

    rename_map.each do |old_path, new_path|
      old_basename = File.basename(old_path)
      new_basename = File.basename(new_path)
      next if old_basename == new_basename

      pattern = /(["'])([^"']*\/)#{Regexp.escape(old_basename)}(["'])/

      if html.match?(pattern)
        html = html.gsub(pattern) do |_match|
          quote = $1
          prefix = $2
          trail = $3
          "#{quote}#{prefix}#{new_basename}#{trail}"
        end
        modified = true
      end
    end

    File.write(html_path, html) if modified
  end
end

def compute_hashed_path(path)
  dir = File.dirname(path)
  basename = File.basename(path, '.*')
  ext = File.extname(path)
  content = File.read(path)
  hash = Digest::MD5.hexdigest(content)[0..7]
  File.join(dir, "#{basename}---#{hash}#{ext}")
end
