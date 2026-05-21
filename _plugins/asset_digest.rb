# frozen_string_literal: true

# Jekyll Asset Digest Filter
#
# Appends a content-based SHA256 hash as a query param to asset paths for cache busting.
#
# Usage:
#   {{ "/assets/css/main.css" | asset_digest | relative_url }}
#   => /assets/css/main.css?6b2c5569
#
# Behavior by file type:
#   - CSS with corresponding .scss source:
#       hash = SHA256(main.scss + all .scss/.sass files in sass_load_paths)
#       This ensures any SASS dependency change invalidates the cache.
#   - CSS without .scss source (e.g., third-party):
#       hash = SHA256(file content)
#   - All other assets (JS, images, fonts, etc.):
#       hash = SHA256(file content)
#
# Results are cached in site.data['asset_digest'] to avoid recomputation within a build.
#
# Example in templates:
#   <link rel="stylesheet" href="{{ '/assets/css/main.css' | asset_digest | relative_url }}">
#   <script src="{{ '/assets/js/app.js' | asset_digest | relative_url }}"></script>

require "digest"

module Jekyll
  module AssetDigestFilter
    def asset_digest(filename)
      site = @context.registers[:site]

      # Return cached result if available
      return site.data["asset_digest"][filename] if site.data.dig("asset_digest", filename)

      result = compute_asset_digest(site, filename)
      site.data["asset_digest"] ||= {}
      site.data["asset_digest"][filename] = result
      result
    end

    private

    def compute_asset_digest(site, filename)
      source_path = site.in_source_dir(filename)

      if filename.end_with?(".css")
        compute_css_digest(site, filename, source_path)
      else
        compute_file_digest(source_path, filename)
      end
    end

    def compute_css_digest(site, filename, source_path)
      scss_file = source_path.sub(/\.css$/, ".scss")

      if File.exist?(scss_file)
        compute_sass_digest(site, filename, scss_file)
      elsif File.exist?(source_path)
        # Third-party CSS: compute hash directly from CSS file
        compute_file_digest(source_path, filename)
      else
        Jekyll.logger.warn "AssetDigest:", "#{filename}: no source found (tried #{scss_file} and #{source_path})"
        filename
      end
    end

    def compute_sass_digest(site, filename, scss_file)
      scss_converter = site.find_converter_instance(Jekyll::Converters::Scss)
      if scss_converter.nil?
        Jekyll.logger.warn "AssetDigest:", "Scss converter not found"
        return filename
      end

      files = [scss_file]
      scss_converter.sass_load_paths.each do |path|
        Dir.glob("#{path}/**/*.scss").each { |f| files << f }
        Dir.glob("#{path}/**/*.sass").each { |f| files << f }
      end

      "#{filename}?#{digest(files)}"
    end

    def compute_file_digest(source_path, filename)
      unless File.exist?(source_path)
        Jekyll.logger.warn "AssetDigest:", "#{filename}: file not found at #{source_path}"
        return filename
      end

      "#{filename}?#{digest([source_path])}"
    end

    def digest(files)
      combined = files.sort.map { |f| File.read(f) }.join
      Digest::SHA256.hexdigest(combined)[0, 8]
    end
  end
end

Liquid::Template.register_filter(Jekyll::AssetDigestFilter)
